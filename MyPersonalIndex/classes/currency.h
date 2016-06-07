#ifndef currency_H
#define currency_H

#include <QString>
#include <QMap>

class QString;
//class QMap;
class QSqlQuery;
class queries;

enum currencyType {
	currencyUSD,
	currencyEUR,
	currencyNIS,
	currencyLast
};

struct PriceData{
	double price;
	bool isTemp; //temp means that it's generated data, not from official source.
};

typedef QMap<int, QMap<currencyType,QMap<currencyType, PriceData> > >   CurrencyMap;
class currencyConverter
{
public:
	
	static double convert(currencyType from, currencyType to, int date = 0); //date = 0 means today
	static QString getDisplayName(currencyType currency);
	static QString getDisplaySign(currencyType currency);
	static QString getCurrencyName( currencyType currency );
	static currencyType getCurrencyTypeFromSign(const QString& displaySign );

	static bool save(const queries &dataSource_);
	static void load(const queries &dataSource_);

private:

	//isTemp - does the price was retrieved from the server or from offline cache of previous date
	static void updatePersistentDataInCache(currencyType from, currencyType to, int date, double price, bool isTemp  );
	//map date to map of currency to currency
	static CurrencyMap dateToCurrencyMap;
};

#endif // currency_H
