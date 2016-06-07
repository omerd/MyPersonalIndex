#include "calculatorNAV.h"
#include <qmath.h>
#include <qnumeric.h>
#include "portfolio.h"
#include "security.h"
#include "account.h"
#include "splits.h"
#include "snapshot.h"
#include "symbol.h"
#include "currency.h"
#include "historicalNAV.h"
#include "functions.h"
#include "tradeDateCalendar.h"
#include "executedTrade.h"
#include "assetAllocationTarget.h"

#ifndef isinf
#define isinf(x) ((x)!=(x))
#endif


#ifndef isnan
#define isnan(x) ((x)!=(x))
#endif

snapshotSecurity calculatorNAV::securitySnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_)
{
    // check today's cache
    //m_securitiesCache.clear();
    snapshotSecurity value = m_securitiesCache.value(date_).value(id_);
    if (!value.isNull())
        return value;

    // check if it needs to be calculated
    security s = portfolio_->securities().value(id_);
    if (!s.includeInCalc() || s.executedTrades().isEmpty())
        return snapshotSecurity(date_);

    // check if prior day is cached
    value = m_securitiesCache.value(
                priorDate_ == 0 ?
                    tradeDateCalendar::previousTradeDate(date_) :
                    priorDate_
            ).value(id_);
	//double priorCostBasis = value.costBasis;
    value.convertToOriginalCurrencyValue();

    splits splitRatio(s.splits(), date_, value.date);

    if (value.date != 0)
        // if there is a split between the cached date (exclusive) and today (inclusive), multiply all existing cached shares by those splits
        value.shares = value.shares * splitRatio.ratio(value.date);

	//bool costChange = false;

    // start loop depending on cached date
	for(QMap<int, executedTrade>::const_iterator i = s.executedTrades().lowerBound(value.date + 1); i != s.executedTrades().constEnd(); ++i)
	{
		if (i.key() > date_)
			break;

		value.shares += i->shares * splitRatio.ratio(i.key());
		//price is always in original currency
		double tradeCost = (i->shares * i->price) + i->commission;
        value.costBasis += tradeCost * currencyConverter::convert((currencyType) s.getSecurityCurrency(), currencyUSD, i.key());
		value.costBasisInOriginalCurrency += tradeCost;
	}
//     for(QMap<int, executedTrade>::const_iterator i = s.executedTrades().lowerBound(value.date + 1); i != s.executedTrades().constEnd(); ++i)
//     {
//        if (i.key() > date_)
//            break;
// 
// 	   costChange = true;
// 	   break;
//     }
// 
// 	if(costChange)
// 	{
// 		value.costBasis = 0;
// 		value.shares = 0;
// 		value.costBasisInOriginalCurrency;
// 		for(QMap<int, executedTrade>::const_iterator i = s.executedTrades().constBegin(); i != s.executedTrades().constEnd(); ++i)
// 		{
// 			if (i.key() > date_)
// 				break;
// 			
// 			value.shares += i->shares * splitRatio.ratio(i.key());
// 			double tradeCost = (i->shares * i->price) + i->commission;
// 			value.costBasis += tradeCost * currencyConverter::convert(currencyUSD, (currencyType) s.getSecurityCurrency(), i.key());
// 			value.costBasisInOriginalCurrency += tradeCost;
// 		}
// 	}


    value.shares = functions::massage(value.shares); // zero out if needed
    value.date = date_;
    value.dividendAmount = value.shares * s.dividend(date_);
    if (s.dividendNAVAdjustment())
        value.dividendAmountNAV = value.dividendAmount;
    value.totalValue = value.shares * s.price(date_);
    value.expenseRatio = s.expenseRatio();
	value.exchangeCurrencyValue = currencyConverter::convert(currencyUSD, (currencyType)s.getSecurityCurrency(), date_);
	if(0 == value.exchangeCurrencyValue)
	{
		value.exchangeCurrencyValue = 1;
	}
	value.isInOriginalCurrency = true;
    account acct = portfolio_->accounts().value(s.account());
    value.setTaxLiability(acct.taxRate(), acct.taxDeferred());

	value.convertToUniversalCurrency();
	//if(!costChange){
    //    value.costBasis = priorCostBasis; //we don't want the cost basis to be cahnged every day.
	//}

    m_securitiesCache[date_].insert(id_, value);
    return value;
}

snapshot calculatorNAV::portfolioSnapshot(const portfolio *portfolio_, int date_, int priorDate_)
{
    snapshot value(date_);

    foreach(const security &s, portfolio_->securities())
        value.add(securitySnapshot(portfolio_, date_, s.id(), priorDate_));

    return value;
}

snapshot calculatorNAV::assetAllocationSnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_)
{
    snapshot value(date_);

    foreach(const security &s, portfolio_->securities())
        if (s.targets().contains(id_))
            value.add(securitySnapshot(portfolio_, date_, s.id(), priorDate_), s.targets().value(id_));

    return value;
}

snapshot calculatorNAV::accountSnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_)
{
    snapshot value(date_);

    foreach(const security &s, portfolio_->securities())
        if (id_ == s.account())
            value.add(securitySnapshot(portfolio_, date_, s.id(), priorDate_));

    return value;
}

snapshot calculatorNAV::symbolSnapshot(int date_, const symbol &key_, int beginDate_)
{
    snapshot value(date_);
    splits splitRatio(key_.splits(), date_, beginDate_);

    value.count = 1;
    value.costBasis = key_.price(beginDate_);
    value.dividendAmount = key_.dividend(date_) * splitRatio.ratio(beginDate_);
    if (key_.includeDividends())
        value.dividendAmountNAV = value.dividendAmount;
    value.totalValue = key_.price(date_) * splitRatio.ratio(beginDate_);

    return value;
}

snapshot calculatorNAV::snapshotByKey(const portfolio *portfolio_, int date_, const objectKeyBase &key_, int beginDate_, int priorDate_)
{
    switch(key_.type())
    {
        case objectType_AA:
            return assetAllocationSnapshot(portfolio_, date_, key_.id(), priorDate_);
        case objectType_Account:
            return accountSnapshot(portfolio_, date_, key_.id(), priorDate_);
        case objectType_Portfolio:
            return portfolioSnapshot(portfolio_, date_, priorDate_);
        case objectType_Security:
            return securitySnapshot(portfolio_, date_, key_.id(), priorDate_);
        case objectType_Symbol:
            return symbolSnapshot(date_, static_cast<const symbol&>(key_), beginDate_);
        case objectType_Trade:
            // not implemented yet, a little too granular...
            return snapshot(0);
    }
    return snapshot(0);
}

int calculatorNAV::beginDateByKey(const portfolio *portfolio_, const objectKeyBase &key_)
{
    switch(key_.type())
    {
        case objectType_AA:
        case objectType_Account:
        case objectType_Portfolio:
        case objectType_Security:
            return portfolio_->startDate();
        case objectType_Symbol:
            return static_cast<const symbol&>(key_).beginDate();
        case objectType_Trade:
            // not implemented yet, a little too granular...
            return 0;
    }
    return 0;
}

int calculatorNAV::endDateByKey(const portfolio *portfolio_, const objectKeyBase &key_)
{
    switch(key_.type())
    {
        case objectType_AA:
        case objectType_Account:
        case objectType_Portfolio:
            return portfolio_->endDate();
        case objectType_Security:
            return static_cast<const security&>(key_).endDate();
        case objectType_Symbol:
            return static_cast<const symbol&>(key_).endDate();
        case objectType_Trade:
            // not implemented yet, a little too granular...
            return 0;
    }
    return 0;
}

double calculatorNAV::dividends(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_, bool inOriginalCurrency)
{
    historicalNAV history = changeOverTime(portfolio_, key_, beginDate_, endDate_, inOriginalCurrency);
    double divi = 0;

    tradeDateCalendar calendar(history.beginDate());
    foreach(int date, ++calendar)
    {
            if (date > history.endDate())
                break;

            navValue currentNAV = history.value(date);
            divi += currentNAV.dividend;
    }

    return divi;
}

double calculatorNAV::xirr(const portfolio *portfolio_, const objectKeyBase &key_, int endDate_, bool inOriginalCurrency)
{
    inOriginalCurrency;
	endDate_ = qMin(endDateByKey(portfolio_, key_), endDate_);
	int beginDate_ = beginDateByKey(portfolio_, key_);

	tradeDateCalendar calendar(beginDate_);
	if (beginDate_ > endDate_ || calendar.date() > endDate_)
		return 0;

	beginDate_ = calendar.date();
	snapshot priorSnapshot = snapshotByKey(portfolio_, beginDate_, key_, beginDate_, 0);

	QList<double> days;
	QList<double> payments;

    if(0 != priorSnapshot.totalValue)
    {
        days.push_back(beginDate_);
        payments.push_back(priorSnapshot.totalValue);
    }

	foreach(int date, ++calendar)
	{
		if (date > endDate_)
			break;

		snapshot currentSnapshot = snapshotByKey(portfolio_, date, key_, beginDate_, priorSnapshot.date);
		double currentCostBasis = 0, priorCostBasis = 0;
		if(key_.type() == objectType_Security)
		{
			currentCostBasis = currentSnapshot.costBasisInOriginalCurrency;
			priorCostBasis = priorSnapshot.costBasisInOriginalCurrency;
		}
		else
		{
			currentCostBasis = currentSnapshot.costBasis;
			priorCostBasis = priorSnapshot.costBasis;
		}



        if(currentCostBasis != priorCostBasis)
		{
			days.push_back(date);
			payments.push_back(currentCostBasis - priorCostBasis);
		}

		priorSnapshot = currentSnapshot;
	}

    if(0 != priorSnapshot.totalValue)
    {
        days.push_back(endDate_);
        double lastTotalValue = priorSnapshot.totalValue;
        if (!priorSnapshot.isInOriginalCurrency)
        {
            lastTotalValue *= priorSnapshot.exchangeCurrencyValue;
        }
        payments.push_back(-1.0 * lastTotalValue);
    }

	return XIRR(payments, days, 0.1);
}

double calculatorNAV::nav(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_, bool inOriginalCurrency)
{
    endDate_ = qMin(endDateByKey(portfolio_, key_), endDate_);
    return changeOverTime(portfolio_, key_, beginDate_, endDate_, inOriginalCurrency).nav(endDate_);
}

historicalNAV calculatorNAV::changeOverTime(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_, bool inOriginalCurrency)
{
    double navValue = 1;
    historicalNAV navHistory;

    beginDate_ = qMax(beginDateByKey(portfolio_, key_), beginDate_);
    endDate_ = qMin(endDateByKey(portfolio_, key_), endDate_);

    tradeDateCalendar calendar(beginDate_);
    if (beginDate_ > endDate_ || calendar.date() > endDate_)
        return navHistory;

    beginDate_ = calendar.date();
    snapshot priorSnapshot = snapshotByKey(portfolio_, beginDate_, key_, beginDate_, 0);
	if(inOriginalCurrency) {
		priorSnapshot.convertToOriginalCurrencyValue();
	}
    navHistory.insert(beginDate_, navValue, priorSnapshot.totalValue, priorSnapshot.dividendAmount); // baseline nav

    foreach(int date, ++calendar)
    {
        if (date > endDate_)
            break;

        snapshot currentSnapshot = snapshotByKey(portfolio_, date, key_, beginDate_, priorSnapshot.date);
		if(inOriginalCurrency) {
			currentSnapshot.convertToOriginalCurrencyValue();
		}

        navValue =
                    change(
                        priorSnapshot.totalValue,
                        currentSnapshot.totalValue,
                        currentSnapshot.costBasis - priorSnapshot.costBasis,
                        currentSnapshot.dividendAmountNAV,
                        navValue
                    );

        navHistory.insert(date, navValue, currentSnapshot.totalValue, currentSnapshot.dividendAmount);
        priorSnapshot = currentSnapshot;
    }

    // take last day's values
    navHistory.costBasis = priorSnapshot.costBasis;
    navHistory.expenseRatio = priorSnapshot.expenseRatio;
    navHistory.taxLiability = priorSnapshot.taxLiability;

    return navHistory;
}

double calculatorNAV::change(double beginValue_, double endValue_, double activity_, double dividends_, double beginNAV_)
{
    if (functions::isZero(endValue_))
        return beginNAV_;

    double nav;
    activity_ -= dividends_;
    if (activity_ < 0)
        nav = (endValue_ - activity_) / (beginValue_ / beginNAV_);
    else
        nav = endValue_ / ((beginValue_ + activity_) / beginNAV_);

    return (isnan(nav) || isinf(nav)) ? beginNAV_ : nav;
}


// Calculates the resulting amount
double calculatorNAV::irrResult(QList<double> values, QList<double> dates, double rate) {
	double r = rate + 1;
	double result = values[0];
	for (double i = 1; i < values.size(); i++)
	{
        double temp = ((dates[i] - dates[0]) * 1.0) / 365.0;
        result += values[i] / pow(r, temp);
	}
	return result;
}

// Calculates the first derivation
double calculatorNAV::irrResultDeriv(QList<double> values, QList<double> dates, double rate) {
	double r = rate + 1;
	double result = 0;
	for (double i = 1; i < values.size(); i++) {
		double frac = ((dates[i] - dates[0]) * 1.0 ) / 365.0;
		result -= frac * values[i] / pow(r, frac + 1);
	}
	return result;
}

double calculatorNAV::XIRR(QList<double> values, QList<double> dates, double guess) {
	// Credits: algorithm inspired by Apache OpenOffice

	if (dates.size() != values.size())
	{
		return qSNaN();
	}


	// Check that values contains at least one positive value and one negative value
	double positive = false;
	double negative = false;
	for (double i = 0; i < values.size(); i++) {
		if (values[i] > 0) positive = true;
		if (values[i] < 0) negative = true;
	}

	// Return error if values does not contain at least one positive value and one negative value
    if (!positive || !negative)
    {
        return qSNaN();
    }

	// Initialize guess and resultRate

	double resultRate = guess;

	// Set maximum epsilon for end of iteration
	double epsMax = 1e-10;

	// Set maximum number of iterations
	double iterMax = 50;

	// Implement Newton's method
	double newRate, epsRate, resultValue;
	double iteration = 0;
	double contLoop = true;
	do {
		resultValue = irrResult(values, dates, resultRate);
		newRate = resultRate - resultValue / irrResultDeriv(values, dates, resultRate);
		epsRate = abs(newRate - resultRate);
		resultRate = newRate;
		contLoop = (epsRate > epsMax) && (abs(resultValue) > epsMax);
	} while (contLoop && (++iteration < iterMax));

	if (contLoop) return qSNaN();

	// Return internal rate of return
	return resultRate;
}
