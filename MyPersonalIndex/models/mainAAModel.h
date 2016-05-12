#ifndef MAINAAMODEL_H
#define MAINAAMODEL_H

#include "mpiViewModelBase.h"
#include "snapshot.h"

class historicalNAV;
class portfolio;
class assetAllocation;
class aaRow: public baseRow
{
public:
    enum {
        row_Description,
        row_CostBasis,
        row_Value,
        row_ValueP,
        row_Gain,
        row_GainP,
        row_Target,
        row_Variance,
        row_RebalanceBand,
        row_Rebalance,
        row_Holdings,
		row_xirr,
		row_dividends
    };

    static const QStringList columns;
    static const QVariantList columnsType;

    aaRow(double change_, double xirr_value, double dividends, double nav_, const snapshot &snapshot_, const snapshot &portfolioSnapshot_, const assetAllocation &aa_, const QList<orderBy> &columnSort_);

    QVariant columnType(int column_) const { return columnsType.at(column_); }
    static QMap<int, QString> fieldNames();

    static QList<baseRow*> getRows(const portfolio &portfolio_, int beginDate_, int endDate_, const snapshot portfolioSnapshot_,
        const QList<orderBy> &columnSort_);

private:
    static baseRow* getRow(const assetAllocation &assetAllocation_, int beginDate_, int endDate_, const portfolio &portfolio_,
        const snapshot &portfolioSnapshot_, const QList<orderBy> &columnSort_);
};

class mainAAModel: public mpiViewModelBase
{
public:
    mainAAModel(const QList<baseRow*> &rows_, const snapshot &portfolioSnapshot_, double portfolioNAV_, const QList<int> &viewableColumns_, QObject *parent_ = 0);

    QVariant data(const QModelIndex &index_, int role_) const;

	//headerData is the data that's displayed in the title of the columns. the header usually contains summary about the portfolio.
    QVariant headerData(int section_, Qt::Orientation orientation_, int role_) const;
	double sumColumnGains(const QString& column) const;

private:
    snapshot m_portfolioSnapshot;
    double m_portfolioNAV;
    double m_target;
};

#endif // MAINAAMODEL_H
