#include "mainSecurityModel.h"
#include <QColor>
#include <cmath>
#include "portfolio.h"
#include "security.h"
#include "assetAllocation.h"
#include "snapshot.h"
#include "calculatorNAV.h"
#include "functions.h"
#include "calculatorAveragePrice.h"
#include "splits.h"
#include "currency.h"
#include "assetAllocationTarget.h"
#include "executedTrade.h"

//enum {
//    row_Active,
//    row_Symbol,
//    row_Note,
//    row_Cash,
//    row_Price,
//    row_Shares,
//    row_Avg,
//    row_Cost,
//    row_Value,
//    row_ValueP,
//    row_Gain,
//    row_GainP,
//    row_NAV,
//    row_Acct,
//    row_AA,
//    row_TaxLiability,
//    row_NetValue,
//    row_ReinvestDividends,
//    row_AdjustForDividends
//};

const QStringList securityRow::columns = QStringList()
                                         << "Active"
                                         << "Symbol"
                                         << "Note"
                                         << "Cash"
                                         << "Closing Price"
                                         << "Shares"
                                         << "Avg Price\nPer Share"
                                         << "Cost Basis"
                                         << "Total Value"
                                         << "% of\nPortfolio"
                                         << "Gain/Loss"
                                         << "Return"
                                         << "Account"
                                         << "Asset Allocation"
                                         << "Tax Liability"
                                         << "After Tax Value"
                                         << "Reinvest\nDividends"
                                         << "Adjust NAV\nFor Dividends"
                                         << "Dividends (gross)"
                                         << "Currency"
                                         << "% of Asset"
                                         << "XIRR";

const QVariantList securityRow::columnsType = QVariantList()
                                              << QVariant(QVariant::Int)
                                              << QVariant(QVariant::String)
                                              << QVariant(QVariant::String)
                                              << QVariant(QVariant::Int)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::String)
                                              << QVariant(QVariant::String)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::Int)
                                              << QVariant(QVariant::Int)
                                              << QVariant(QVariant::Double)
                                              << QVariant(QVariant::String)
                                              << QVariant(QVariant::String)
                                              << QVariant(QVariant::Double);

securityRow::securityRow(double change_, double xirr_value, double percentageOfAssetAllocation, double dividend, double nav_, const snapshotSecurity &snapshot_, account::costBasisMethod costBasis_, const snapshot &portfolioSnapshot_,
    const security &security_, const QString &account_, const QString &assetAllocation_, const QList<orderBy> &columnSort_):
    baseRow(columnSort_)
{
    //    row_Active,
    this->values.append((int)security_.includeInCalc());
    //    row_Symbol,
    this->values.append(security_.description());
    //    row_Note,
    this->values.append(security_.note());
    //    row_Cash,
    this->values.append((int)security_.cashAccount());
    //    row_Price,
    double price = security_.price(snapshot_.date);
    this->values.append(functions::isZero(price) ? QVariant() : price);
    //    row_Shares,
    this->values.append(snapshot_.shares);
    //    row_Avg,

    double avgPrice = calculatorAveragePrice::calculate(snapshot_.date, security_.executedTrades(), costBasis_, splits(security_.splits(), snapshot_.date));

    // cash should always be computed as average
    this->values.append(
        functions::isZero(snapshot_.shares) ?
            QVariant() :
            security_.cashAccount() ? 1 : avgPrice
    );
    //    row_Cost,
    this->values.append(snapshot_.costBasisInOriginalCurrency);
    //    row_Value,
    this->values.append(snapshot_.totalValue);
    //    row_ValueP, percentage from portfolio
    this->values.append(functions::checkDivisor(portfolioSnapshot_.totalValue, snapshot_.totalValue / snapshot_.exchangeCurrencyValue));


    //    row_Gain,
    //this->values.append(snapshot_.totalValue - snapshot_.costBasis);
    this->values.append(change_);

    //    row_GainP,
    //double gainFromValue = qMax(snapshot_.totalValue - change_ , snapshot_.costBasisInOriginalCurrency);
	//snapshot_.totalValue == 0.0 ? this->values.append(nav_ - 1) : this->values.append( (change_ / (gainFromValue)));
	this->values.append(nav_);

    //    row_Acct,
    this->values.append(account_);
    //    row_AA,
    this->values.append(assetAllocation_);
    //    row_TaxLiability,
    this->values.append(snapshot_.taxLiability);
    //    row_NetValue,
    this->values.append(snapshot_.totalValue - snapshot_.taxLiability);
    //    row_ReinvestDividends,
    this->values.append((int)security_.dividendReinvestment());
    //    row_AdjustForDividends
    this->values.append((int)security_.dividendNAVAdjustment());
    //    row_Dividends Received (net)
    this->values.append(dividend);
    //    row_Currency
    this->values.append(currencyConverter::getDisplaySign((currencyType)security_.getSecurityCurrency()));
    //    P_OfAA - percentage of asset allocation
    this->values.append(functions::doubleToPercentage(percentageOfAssetAllocation));
    //xirr
    this->values.append(xirr_value);
}

QList<baseRow*> securityRow::getRows(const portfolio &portfolio_, int beginDate_, int endDate_, const snapshot &portfolioSnapshot_,
    const QList<orderBy> &columnSort_)
{
    QList<baseRow*> returnList;

    foreach(const security &sec, portfolio_.securities())
    {
        if (sec.hidden())
            continue;

        QStringList aaDescription;
        for(QMap<int, double>::const_iterator i = sec.targets().constBegin(); i != sec.targets().constEnd(); ++i)
            aaDescription.prepend(
                QString("%1 - %2").arg(i.key() == UNASSIGNED ? "(Unassigned)" : portfolio_.assetAllocations().value(i.key()).displayText(),
                                        functions::doubleToPercentage(i.value()))
            );


        snapshotSecurity snapSecurityInOriginalCurrency = portfolio_.securitySnapshot(endDate_, sec.id());

		double _dividends = portfolio_.dividend(sec, beginDate_, endDate_, true);

        double percentageOfAssetAllocation = 0;
        if(sec.targets().count() == 1)
        {
            int assetKey = sec.targets().constBegin().key();
            snapshot assetAllocationSnapshot = portfolio_.assetAllocationSnapshot(endDate_,assetKey);
            percentageOfAssetAllocation = snapSecurityInOriginalCurrency.totalValue / assetAllocationSnapshot.totalValue;
			if(0 == snapSecurityInOriginalCurrency.totalValue)
			{
				percentageOfAssetAllocation = 0;
			}
        }
        snapSecurityInOriginalCurrency.convertToOriginalCurrencyValue();
        snapshotSecurity temp = portfolio_.securitySnapshot(beginDate_, sec.id());
        temp.convertToOriginalCurrencyValue();
        double change_ = 0;
        change_ = snapSecurityInOriginalCurrency.getGains() - temp.getGains();
		double chnageInPercentage = 0;

		//double gainFromValue = qMax(snapNow.totalValue - change_ , snapNow.costBasis);
		double gainFromValue = snapSecurityInOriginalCurrency.totalValue - change_ ;
		if(sec.dividendNAVAdjustment())
		{
			change_ += _dividends;
		}

		chnageInPercentage = change_ ? (change_ / (gainFromValue )) : 0;


        returnList.append(
            new securityRow(
                change_,
                portfolio_.xirr(sec, endDate_, true),
                percentageOfAssetAllocation,
                _dividends,
                chnageInPercentage,//portfolio_.nav(sec, beginDate_, endDate_, true),
                snapSecurityInOriginalCurrency,
                portfolio_.accounts().value(sec.account()).costBasis(),
                portfolioSnapshot_,
                sec,
                sec.account() == UNASSIGNED ? QString() : portfolio_.accounts().value(sec.account()).displayText(),
                aaDescription.join(", "),
                columnSort_
            )
        );
    }

    return returnList;
}

QMap<int, QString> securityRow::fieldNames()
{
    QMap<int, QString> names;

    for (int i = 0; i < columns.count(); ++i)
        names[i] = functions::removeNewLines(columns.at(i));

    return names;
}

mainSecurityModel::mainSecurityModel(const QList<baseRow*> &rows_, const snapshot &portfolioSnapshot_, double portfolioNAV_, const QList<int> &viewableColumns_, QObject *parent_ ):
    mpiViewModelBase(rows_, viewableColumns_, false, parent_),
    m_portfolioSnapshot(portfolioSnapshot_),
    m_portfolioNAV(portfolioNAV_)
{
}

bool mainSecurityModel::isRowWithShares(const QModelIndex &index_)const{
    baseRow* row  = m_rows.at(index_.row());
    int index = -1;
    for(int i = 0 ; i < securityRow::columns.size(); i++)
    {
        QString rowName = securityRow::columns[i];
        if (rowName.compare("Shares") == 0)
        {
            index  = i;
        }
    }
    if(-1 == index)
    {
        return true;
    }
    return 0 != row->values.at(index).toDouble();
}

QVariant mainSecurityModel::data(const QModelIndex &index_, int role_) const
{
    if (!index_.isValid())
        return QVariant();

    int column = m_viewableColumns.at(index_.column());
    QVariant value = m_rows.at(index_.row())->values.at(column);

    if(role_ == Qt::UserRole) //retrieve information about symbl name for double click
    {
        baseRow* row  = m_rows.at(index_.row());
        int index = -1;
        for(int i = 0 ; i < securityRow::columns.size(); i++)
        {
            QString rowName = securityRow::columns[i];
            if (rowName.compare("Symbol") == 0)
            {
                index  = i;
            }
        }
        if(-1 == index)
        {
            return QVariant();
        }
        return row->values.at(index);
    }

    if (role_ == Qt::DisplayRole)
    {
        if (value.isNull() || column == securityRow::row_Active || column == securityRow::row_Cash ||column == securityRow::row_ReinvestDividends || column == securityRow::row_AdjustForDividends)
            return QVariant();

        switch (column)
        {
            case securityRow::row_Avg:
            case securityRow::row_Cost:
            case securityRow::row_Gain:
            case securityRow::row_Price:
            case securityRow::row_Value:
            case securityRow::row_TaxLiability:
            case securityRow::row_NetValue:
                return functions::doubleToCurrency(value.toDouble(), ""); //You can, however, find the currency instead of passing empty string. find it in the same way we find the symbol name
            case securityRow::row_GainP:
            case securityRow::row_ValueP:
            case securityRow::row_xirr:
                return functions::doubleToPercentage(value.toDouble());
            case securityRow::row_Shares:
				{
					double intpart;
					if(std::modf(value.toDouble(), &intpart) == 0.0)
						return functions::doubleToLocalFormat(value.toDouble(), 0);
					else
					{
						return functions::doubleToLocalFormat(value.toDouble(), 4);
					}
				}
        }

        return value;
    }

    if (role_ == Qt::CheckStateRole && (column == securityRow::row_Active || column == securityRow::row_Cash || column == securityRow::row_ReinvestDividends || column == securityRow::row_AdjustForDividends))
    {
        return value.toInt() == 1 ? Qt::Checked : Qt::Unchecked;
    }

    if (role_ == Qt::TextColorRole)
    {
        if(!isRowWithShares(index_))
        {
            return qVariantFromValue(QColor(Qt::gray));
        }

        if((column == securityRow::row_GainP || column == securityRow::row_xirr)){
            return functions::isZero(value.toDouble()) ?
                QVariant() :
                value.toDouble() > 0 ?
                    qVariantFromValue(QColor(Qt::darkGreen)) :
                    qVariantFromValue(QColor(Qt::red));
        }
    }

    return QVariant();
}

QVariant mainSecurityModel::headerData(int section_, Qt::Orientation orientation_, int role_) const
{
     if (section_ >= m_viewableColumns.count())
        return QVariant();

    if (role_ == Qt::TextAlignmentRole)
        return (int)Qt::AlignLeft | Qt::AlignVCenter;

    if (orientation_ != Qt::Horizontal || role_ != Qt::DisplayRole)
        return QVariant();

    int column = m_viewableColumns.at(section_);
    QString extra;
	double gains = sumPortfolioGains();
	double gainFromValue = qMax(m_portfolioSnapshot.totalValue - gains , m_portfolioSnapshot.costBasis);
    switch(column)
    {
        case securityRow::row_Cost:
            extra = QString("\n[%1]").arg(functions::doubleToCurrency(m_portfolioSnapshot.costBasis));
            break;
        case securityRow::row_Value:
            extra = QString("\n[%1]").arg(functions::doubleToCurrency(m_portfolioSnapshot.totalValue));
            break;
        case securityRow::row_Gain:
			{
				extra = QString("\n[%1]").arg(functions::doubleToCurrency(gains));
			}
            
            break;
        case securityRow::row_GainP:
			extra = QString("\n[%1]").arg(functions::doubleToPercentage(gains / gainFromValue));
            //extra = QString("\n[%1]").arg(functions::doubleToPercentage(m_portfolioNAV - 1));
            break;
        case securityRow::row_TaxLiability:
            extra = QString("\n[%1]").arg(functions::doubleToCurrency(m_portfolioSnapshot.taxLiability));
            break;
        case securityRow::row_NetValue:
            extra = QString("\n[%1]").arg(functions::doubleToCurrency(m_portfolioSnapshot.totalValue - m_portfolioSnapshot.taxLiability));
            break;
    }

    return QString(securityRow::columns.at(column)).append(extra);
}

double mainSecurityModel::sumPortfolioGains() const
{
	int indexGainLoss = -1;
	int indexCurrency = -1;
	for(int i = 0 ; i < securityRow::columns.size(); i++)
	{
		QString rowName = securityRow::columns[i];
		if (rowName.compare("Gain/Loss") == 0)
		{
			indexGainLoss  = i;
		}
	}
	for(int i = 0 ; i < securityRow::columns.size(); i++)
	{
		QString rowName = securityRow::columns[i];
		if (rowName.compare("Currency") == 0)
		{
			indexCurrency  = i;
		}
	}
	if(-1 == indexGainLoss || -1 == indexCurrency)
	{
		return 0;
	}

	double gains = 0;
	for(int i = 0; i < m_rows.size(); i++)
	{
		baseRow* row  = m_rows.at(i);
		currencyType type =  currencyConverter::getCurrencyTypeFromSign(row->values.at(indexCurrency).toString());
		double rate = currencyConverter::convert(type, currencyUSD);
		gains += rate * row->values.at(indexGainLoss).toDouble();
	}
	return gains;
}
