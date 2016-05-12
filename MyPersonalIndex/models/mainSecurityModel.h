#ifndef MAINHOLDINGSMODEL_H
#define MAINHOLDINGSMODEL_H

#include "mpiViewModelBase.h"
#include "snapshot.h"
#include "account.h"

class historicalNAV;
class security;
class portfolio;
class securityRow: public baseRow
{
public:
    enum {
        row_Active,
        row_Symbol,
        row_Note,
        row_Cash,
        row_Price,
        row_Shares,
        row_Avg,
        row_Cost,
        row_Value,
        row_ValueP,
        row_Gain,
        row_GainP,
        row_Acct,
        row_AA,
        row_TaxLiability,
        row_NetValue,
        row_ReinvestDividends,
        row_AdjustForDividends,
        row_DividendsNAV,
		row_Currency,
		row_P_OfAA,
		row_xirr,
    };

    static const QStringList columns;
    static const QVariantList columnsType;

    securityRow(double change_, double xirr_value, double percentageOfAssetAllocation, double dividend, double nav_, const snapshotSecurity &snapshot_, account::costBasisMethod costBasis_, const snapshot &portfolioSnapshot_,
        const security &security_, const QString &account_, const QString &assetAllocation_, const QList<orderBy> &columnSort_);

    QVariant columnType(int column) const { return columnsType.at(column); }
    static QMap<int, QString> fieldNames();

    static QList<baseRow*> getRows(const portfolio &portfolio_, int beginDate_, int endDate_, const snapshot &portfolioSnapshot_,
        const QList<orderBy> &columnSort_);
};

class mainSecurityModel: public mpiViewModelBase
{
public:
    mainSecurityModel(const QList<baseRow*> &rows_, const snapshot &portfolioSnapshot_, double portfolioNAV_, const QList<int> &viewableColumns_, QObject *parent_ = 0);

    QVariant data(const QModelIndex &index_, int role_) const;

	//headerData is the data that's displayed in the title of the columns. the header usually contains summary about the portfolio.
    QVariant headerData(int section_, Qt::Orientation orientation_, int role_) const;

private:

	bool isRowWithShares(const QModelIndex &index_) const;
	double sumPortfolioGains() const;

    snapshot m_portfolioSnapshot;
    double m_portfolioNAV;
};


#endif // MAINHOLDINGSMODEL_H
