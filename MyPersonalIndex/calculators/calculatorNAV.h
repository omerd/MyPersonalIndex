#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include <QHash>
#include <Qlist>

class portfolio;
class symbol;
class objectKeyBase;
class snapshot;
class snapshotSecurity;
class security;
class historicalNAV;
class calculatorNAV
{
public:
    /* Note:
       Each instance of this class should only be called with a portfolio pointer with the same underlying
       shared data.  Since the portfolio class is a shared data pointer, there is no constant pointer to a
       parent object as the underlying data is passed around.  Thus it has to be passed into each call.
    */

    void clearCache() {  m_securitiesCache.clear(); }

    // priorDate is a shortcut to pull data from cache, it is optional but can greatly speed up calculations
    snapshotSecurity securitySnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_ = 0);
    snapshot portfolioSnapshot(const portfolio *portfolio_, int date_, int priorDate_ = 0);
    snapshot assetAllocationSnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_ = 0);
    snapshot accountSnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_ = 0);
    // since there are no trades to start from like the methods above, a begin date is required for splits
    snapshot symbolSnapshot(int date_, const symbol &key_, int beginDate_);

    double nav(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_, bool inOriginalCurrency = false);
    double xirr(const portfolio *portfolio_, const objectKeyBase &key_, int endDate_, bool inOriginalCurrency = false);
    double dividends(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_, bool inOriginalCurrency = false);
    historicalNAV changeOverTime(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_, bool inOriginalCurrency = false);

private:
    QHash<int, QHash<int, snapshotSecurity> > m_securitiesCache;

    snapshot snapshotByKey(const portfolio *portfolio_, int date_, const objectKeyBase &key_, int beginDate_, int priorDate_);
    int beginDateByKey(const portfolio *portfolio_, const objectKeyBase &key_);
    int endDateByKey(const portfolio *portfolio_, const objectKeyBase &key_);

	//activity is the diffrence between the cost basis of the end and the beginning
    static double change(double beginValue_, double endValue_, double activity_, double dividends_, double beginNAV_);

	double XIRR(QList<double> values, QList<double> dates, double guess);
	double irrResultDeriv(QList<double> values, QList<double> dates, double rate);
	double irrResult(QList<double> values, QList<double> dates, double rate) ;
};

#endif // CALCULATIONS_H
