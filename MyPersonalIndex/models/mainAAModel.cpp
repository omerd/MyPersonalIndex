#include "mainAAModel.h"
#include <QColor>
#include <qmath.h>
#include <qnumeric.h>
#include "functions.h"
#include "assetAllocation.h"
#include "calculatorNAV.h"
#include "historicalNAV.h"
#include "portfolio.h"
#include "security.h"
#include "assetAllocationTarget.h"

//enum {
//    row_Description,
//    row_CostBasis,
//    row_Value,
//    row_ValueP,
//    row_Gain,
//    row_GainP,
//    row_Target,
//    row_Variance,
//    row_RebalanceBand,
//    row_Rebalance,
//    row_Holdings
//};

const QStringList aaRow::columns = QStringList()
                                   << "Asset Class"
                                   << "Cost Basis"
                                   << "Total Value"
                                   << "% of Portfolio"
                                   << "Gain/Loss"
                                   << "Return"
                                   << "Target"
                                   << "Variance"
                                   << "Rebalance\nBand"
                                   << "Rebalance\nAmount"
                                   << "# Holdings"
								   << "XIRR"
								   << "Dividends";;

const QVariantList aaRow::columnsType = QVariantList()
                                        << QVariant(QVariant::String)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Double)
                                        << QVariant(QVariant::Int)
										<< QVariant(QVariant::Double)
										<< QVariant(QVariant::Double);;


aaRow::aaRow(double change_, double xirr_value, double dividends, double nav_, const snapshot &snapshot_, const snapshot &portfolioSnapshot_, const assetAllocation &aa_, const QList<orderBy> &columnSort_):
    baseRow(columnSort_)
{    
    //    row_Description,
    this->values.append(aa_.displayText());
    //    row_CostBasis,
    this->values.append(snapshot_.costBasis);
    //    row_Value,
    this->values.append(snapshot_.totalValue);
    //    row_ValueP,
    this->values.append(functions::checkDivisor(portfolioSnapshot_.totalValue, snapshot_.totalValue));
    //    row_Gain,
    this->values.append(change_);
    //    row_GainP,
    this->values.append(nav_);
    //    row_Target,
    this->values.append(functions::isZero(aa_.target()) ? QVariant() : aa_.target());
    //    row_Variance,
    this->values.append(
        functions::isZero(portfolioSnapshot_.totalValue) || functions::isZero(aa_.target()) ?
            QVariant() :
            aa_.threshold() == assetAllocation::thresholdMethod_AA ?
                (snapshot_.totalValue / (portfolioSnapshot_.totalValue * aa_.target())) - 1 :
                (snapshot_.totalValue / portfolioSnapshot_.totalValue) - aa_.target()
        );
    //    row_RebalanceBand,
    this->values.append(functions::isZero(aa_.rebalanceBand()) ? QVariant() : aa_.rebalanceBand());
    //    row_Rebalance,
    this->values.append(
        functions::isZero(portfolioSnapshot_.totalValue) || functions::isZero(aa_.target()) ?
            QVariant() :
            -1 * portfolioSnapshot_.totalValue * ((snapshot_.totalValue / portfolioSnapshot_.totalValue) - aa_.target())
        );
    //    row_Holdings
    this->values.append(snapshot_.count);

	//    row_xirr
	this->values.append(xirr_value);

	//	row_dividends
	this->values.append(dividends);
}

QMap<int, QString> aaRow::fieldNames()
{
    QMap<int, QString> names;

    for (int i = 0; i < columns.count(); ++i)
        names[i] = functions::removeNewLines(columns.at(i));

    return names;
}

QList<baseRow*> aaRow::getRows(const portfolio &portfolio_, int beginDate_, int endDate_, const snapshot portfolioSnapshot_,
    const QList<orderBy> &columnSort_)
{
    QList<baseRow*> returnList;

    foreach(const assetAllocation &aa, portfolio_.assetAllocations())
    {
        if (aa.hidden())
            continue;

        returnList.append(getRow(aa, beginDate_, endDate_, portfolio_, portfolioSnapshot_, columnSort_));
    }

    // check if any securities have an unassigned value
    snapshot unassigned = portfolio_.assetAllocationSnapshot(endDate_, UNASSIGNED);
    if (unassigned.count != 0)
    {
        assetAllocation aa;
        aa.setDescription("(Unassigned)");
        returnList.append(getRow(aa, beginDate_, endDate_, portfolio_, portfolioSnapshot_, columnSort_));
    }

    return returnList;
}

baseRow* aaRow::getRow(const assetAllocation &assetAllocation_, int beginDate_, int endDate_, const portfolio &portfolio_, const snapshot &portfolioSnapshot_,
    const QList<orderBy> &columnSort_)
{
	double divindend = 0;
	double divindendForReturnCalc = 0;
	foreach(const security &sec, portfolio_.securities())
	{
		if (sec.targets().contains(assetAllocation_.id()))
		{
			double divid = portfolio_.dividend(sec, beginDate_, endDate_);
			divindend += divid;
			if(sec.dividendNAVAdjustment())
			{
				divindendForReturnCalc += divid;
			}
		}
		
	}

    snapshot snapNow = portfolio_.assetAllocationSnapshot(endDate_, assetAllocation_.id());
    snapshot snapBegin = portfolio_.assetAllocationSnapshot(beginDate_, assetAllocation_.id());
	double change_ = 0;
	change_ = snapNow.getGains() - snapBegin.getGains();
	double chnageInPercentage = 0;

	double gainFromValue = snapNow.totalValue - change_ ;
	change_ += divindendForReturnCalc;
	chnageInPercentage = change_ ? (change_ / (gainFromValue )) : 0;


    return new aaRow(
		change_,
		portfolio_.xirr(assetAllocation_, endDate_),
		divindend,
        //portfolio_.nav(assetAllocation_, beginDate_, endDate_) - 1,
		chnageInPercentage,
        portfolio_.assetAllocationSnapshot(endDate_, assetAllocation_.id()), portfolioSnapshot_, assetAllocation_, columnSort_
    );
}

mainAAModel::mainAAModel(const QList<baseRow*> &rows_, const snapshot &portfolioSnapshot_, double portfolioNAV_, const QList<int> &viewableColumns_, QObject *parent_):
    mpiViewModelBase(rows_, viewableColumns_, false, parent_),
    m_portfolioSnapshot(portfolioSnapshot_),
    m_portfolioNAV(portfolioNAV_),
    m_target(0)
{
    foreach(const baseRow *r, rows_)
        m_target += r->values.at(aaRow::row_Target).toDouble();
}

QVariant mainAAModel::data(const QModelIndex &index_, int role_) const
{
    if (!index_.isValid())
        return QVariant();

    int column = m_viewableColumns.at(index_.column());
    QVariant value = m_rows.at(index_.row())->values.at(column);

    if (role_ == Qt::DisplayRole)
    {
        if (value.isNull())
            return QVariant();

        switch (column)
        {
            case aaRow::row_Value:
            case aaRow::row_CostBasis:
            case aaRow::row_Gain:
            case aaRow::row_Rebalance:
			case aaRow::row_dividends:
                return functions::doubleToCurrency(value.toDouble());
            case aaRow::row_ValueP:
            case aaRow::row_Target:
            case aaRow::row_Variance:
            case aaRow::row_GainP:
            case aaRow::row_RebalanceBand:
			case aaRow::row_xirr:
                return functions::doubleToPercentage(value.toDouble());
        }

        return value;
    }

    if (role_ == Qt::TextColorRole && column == aaRow::row_Variance)
    {
        double threshold = m_rows.at(index_.row())->values.at(aaRow::row_RebalanceBand).toDouble();
        if (functions::isZero(threshold))
            return QVariant();

        return qAbs(value.toDouble()) <= threshold ? QVariant() :
            value.toDouble() > threshold ? qVariantFromValue(QColor(Qt::darkGreen)) : qVariantFromValue(QColor(Qt::red));
    }

    if (role_ == Qt::TextColorRole && (column == aaRow::row_GainP || column == aaRow::row_xirr))
        return functions::isZero(value.toDouble()) ?
            QVariant() :
            value.toDouble() > 0 ?
                qVariantFromValue(QColor(Qt::darkGreen)) :
                qVariantFromValue(QColor(Qt::red));

    return QVariant();
}

QVariant mainAAModel::headerData(int section_, Qt::Orientation orientation_, int role_) const
{
     if (section_ >= m_viewableColumns.count())
        return QVariant();

    if (role_ == Qt::TextAlignmentRole)
        return (int)Qt::AlignLeft | Qt::AlignVCenter;

    if (orientation_ != Qt::Horizontal || role_ != Qt::DisplayRole)
        return QVariant();

    int column = m_viewableColumns.at(section_);
    QString extra;
	double gains = sumColumnGains("Gain/Loss");
	double dividends = sumColumnGains("Dividends");
	double gainFromValue = m_portfolioSnapshot.totalValue - gains - dividends;
	double chnageInPercentage = gains? (gains / (gainFromValue)) : 0;

    switch(column)
    {
        case aaRow::row_CostBasis:
            extra = QString("\n[%1]").arg(functions::doubleToCurrency(m_portfolioSnapshot.costBasis));
            break;
        case aaRow::row_Value:
            extra = QString("\n[%1]").arg(functions::doubleToCurrency(m_portfolioSnapshot.totalValue));
            break;
        case aaRow::row_Gain:
            //extra = QString("\n[%1]").arg(functions::doubleToCurrency(m_portfolioSnapshot.totalValue - m_portfolioSnapshot.costBasis));
			extra = QString("\n[%1]").arg(functions::doubleToCurrency(gains));
            break;
        case aaRow::row_GainP:
            //extra = QString("\n[%1]").arg(functions::doubleToPercentage(m_portfolioNAV - 1));
			extra = QString("\n[%1]").arg(functions::doubleToPercentage(chnageInPercentage));
            break;
        case aaRow::row_Target:
            extra = QString("\n[%1]").arg(functions::doubleToPercentage(m_target));
            break;
    }

    return QString(aaRow::columns.at(column)).append(extra);
}

double mainAAModel::sumColumnGains(const QString& column ) const
{
	int indexGainLoss = -1;
	for(int i = 0 ; i < aaRow::columns.size(); i++)
	{
		QString rowName = aaRow::columns[i];
		if (rowName.compare(column) == 0)
		{
			indexGainLoss  = i;
		}
	}

	//Assume currency is USD

	if(-1 == indexGainLoss)
	{
		return 0;
	}

	double gains = 0;
	for(int i = 0; i < m_rows.size(); i++)
	{
		baseRow* row  = m_rows.at(i);
		gains += row->values.at(indexGainLoss).toDouble();
	}
	return gains;
}
