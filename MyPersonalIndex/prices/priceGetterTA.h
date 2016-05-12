#ifndef PRICEGETTERTA_H
#define PRICEGETTERTA_H

#include "priceGetter.h"

template<class T>
class QList;
class QByteArray;
class QUrl;
class QDate;
class priceGetterTA : public priceGetter
{
public:
    bool isValidSymbol(const QString &symbol_) const;

    int getPrices(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const;
    int getDividends(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const;
    int getSplits(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const;

private:
	static const char stockPrices = 'd';

	static QList<QByteArray> downloadFile(const QUrl &url_, bool splitResultByLineBreak = true);
	static QString getCSVAddress(const QString &symbol_, const QDate &beginDate_, const QDate &endDate_, const QString &type_);
};

#endif // PRICEGETTERTA_H
