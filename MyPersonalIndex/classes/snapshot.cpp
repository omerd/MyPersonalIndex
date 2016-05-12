#include "snapshot.h"
#include "functions.h"

void snapshot::add(const snapshot &other_, double multiplier_)
{
    if (other_.isNull())
        return;
	//Always add in universal currency mode
	if(isInOriginalCurrency)
	{
		convertCurrencies(1.0 / this->exchangeCurrencyValue);
	}

    double startingTotalValue = this->totalValue;

    this->costBasis += other_.costBasis * multiplier_ ;
	this->costBasisInOriginalCurrency += other_.costBasisInOriginalCurrency * multiplier_ ;
    this->totalValue += other_.totalValue * multiplier_;
    this->taxLiability += other_.taxLiability * multiplier_;
    this->dividendAmount += other_.dividendAmount * multiplier_;
    this->dividendAmountNAV += other_.dividendAmountNAV * multiplier_;

    if (functions::isZero(this->totalValue))
        this->expenseRatio = 0;
    else
        this->expenseRatio = // allocate based on total value percentage
            (
                (startingTotalValue / this->totalValue) * this->expenseRatio
            )
                +
            (
                ((other_.totalValue * multiplier_) / this->totalValue) * other_.expenseRatio
            );

    ++this->count;
}

void snapshot::convertToOriginalCurrencyValue()
{
	if(isInOriginalCurrency)
	{
		return;
	}

	convertCurrencies(this->exchangeCurrencyValue);

	isInOriginalCurrency = true;
}


void snapshot::convertToUniversalCurrency()
{
	if(!isInOriginalCurrency)
	{
		return;
	}

	convertCurrencies(1.0 / this->exchangeCurrencyValue);
	isInOriginalCurrency = false;

}


void snapshot::convertCurrencies( double multiplier )
{
	//this->costBasis *= multiplier;
	this->totalValue *= multiplier;
	this->taxLiability *= multiplier;
	this->dividendAmount *= multiplier;
	this->dividendAmountNAV *= multiplier;
	//this->expenseRatio /= multiplier;
}

double snapshot::getGains()
{
	if(isInOriginalCurrency)
	{
		return totalValue - costBasisInOriginalCurrency;
	}
	else
	{
		return totalValue - costBasis;
	}
}

void snapshotSecurity::setTaxLiability(double taxRate_, bool taxDeferred_)
{
    if (functions::isZero(taxRate_ ))
        return;

    if (taxDeferred_)
        taxLiability = totalValue * taxRate_;
    else if (totalValue > costBasisInOriginalCurrency)
        taxLiability = (totalValue - costBasisInOriginalCurrency) * taxRate_;
}
