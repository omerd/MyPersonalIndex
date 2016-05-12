#include "currency.h"

#include <QUrl>
#include <QDate>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QObject>
#include <QSqlQuery>
#include "queries.h"



CurrencyMap  currencyConverter::dateToCurrencyMap;

double getResult(currencyType from, currencyType to, int date )
{
    QDate d = QDate::fromJulianDay(date);
    QString url =  QString("http://127.0.0.1:8016/?date=%1&currency_to=%2&currency_from=%3").arg(
        d.toString("yyyy-MM-dd"), currencyConverter::getCurrencyName(to), currencyConverter::getCurrencyName(from));

    QNetworkAccessManager manager;
    QEventLoop loop;
    QUrl _url(url);
    QNetworkRequest request(_url);
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    loop.exec();

    QList<QByteArray> lines;
    if (reply->error() == QNetworkReply::NoError)
    {
        lines.append(reply->readAll());
    }

    delete reply;
	if(lines.size() > 0 )
	{
		return lines.at(0).toDouble();
	}

	return 0;
}

bool hasValue(const CurrencyMap& currMap, currencyType from, currencyType to, int date)
{
	return currMap.contains(date) &&
		currMap[date].contains(from) &&
		currMap[date][from].contains(to);
}

bool hasOfficialCurrencyValue(const CurrencyMap& currMap, currencyType from, currencyType to, int date)
{
	for(int i = 0; i < 10; i ++)
	{
        if(hasValue(currMap, from, to, date + i)) //if a later date has an official value, so probably date was a day without currency trading. so we do want to cache the temp value
		{
			if( !currMap[date][from][to].isTemp)
			{
				return true;
			}
		}
	}
	return false;
}

double currencyConverter::convert( currencyType from, currencyType to, int date )
{
    if (to == from)
    {
        return 1;
    }
    if( 0 == date)
    {
        date = QDate::currentDate().toJulianDay();
    }

    if (hasValue(dateToCurrencyMap, from, to, date))
    {
        return dateToCurrencyMap[date][from][to].price;
    }

    switch(to)
    {
    case currencyUSD:
    case currencyNIS:
    case currencyEUR:
        {
			PriceData data = {0};
			data.price =  getResult(from, to, date);
			if(0 != data.price){
				data.isTemp = false;
				//0 is not cached. 0 means that the server is offline or there is a bug in the server. we don't want this information to be saved forever in the db.
				dateToCurrencyMap[date][from][to] = data;

                PriceData converse;
                converse.price = 1.0 / data.price;
                converse.isTemp = false;
                dateToCurrencyMap[date][to][from] = converse;
				
			}
			else
			{
				int currentDate = date-1;
				for(int i = 0;		i < 15;		i ++, currentDate--)
				{
					if (hasValue(dateToCurrencyMap, from, to, currentDate))
					{
						data.price = dateToCurrencyMap[currentDate][from][to].price;
						data.isTemp = true;
						dateToCurrencyMap[date][from][to] = data;
                        break;
					}
				}
			}
        return data.price;
        }
    default:
        return 0;
    }
}

QString currencyConverter::getCurrencyName( currencyType currency )
{
    switch(currency)
    {
    case currencyNIS:
        return "ILS";
    case currencyUSD:
        return "USD";
    case currencyEUR:
        return "EUR";
    default:
        return "";
    }
}

QString currencyConverter::getDisplayName( currencyType currency )
{
    switch(currency)
    {
    case currencyNIS:
        return "Israeli Shekel";
    case currencyUSD:
        return "US Dollar";
    case currencyEUR:
        return "EURO";
    default:
        return "";
    }
}

QString currencyConverter::getDisplaySign( currencyType currency )
{
    switch(currency)
    {
    case currencyNIS:
        return "NIS";
    case currencyUSD:
        return "$";
    case currencyEUR:
        return "EUR";
    default:
        return "";
    }
}

bool currencyConverter::save( const queries &dataSource_ )
{
    if (!dataSource_.deleteTable(queries::table_Currency))
        return false;

    for(CurrencyMap::iterator i = dateToCurrencyMap.begin(); i != dateToCurrencyMap.end(); i++)
    {
        QMap<currencyType,QMap<currencyType, PriceData> > & curr_to_curr_to_value = i.value();
        for(QMap<currencyType,QMap<currencyType, PriceData> >::iterator j = curr_to_curr_to_value.begin(); j != curr_to_curr_to_value.end(); j++)
        {
            QMap<currencyType, PriceData> & curr_to_value = j.value();
            for(QMap<currencyType, PriceData>::iterator k = curr_to_value.begin(); k != curr_to_value.end(); k++)
            {
				if(!hasOfficialCurrencyValue(dateToCurrencyMap, j.key(), k.key(),i.key()))
				{
					continue;
				}

                QMap<QString, QVariant> values;
                values.insert(queries::portfolioCurrencyColumns.at(queries::currencyColumns_date), i.key());
                values.insert(queries::portfolioCurrencyColumns.at(queries::currencyColumns_currency_from), j.key() );
                values.insert(queries::portfolioCurrencyColumns.at(queries::currencyColumns_currency_to), k.key() );
                values.insert(queries::portfolioCurrencyColumns.at(queries::currencyColumns_currency_value), k.value().price );

                if(! dataSource_.insert(queries::table_Currency, values))
                    return false;
            }

        }
    }

    return true;
}

void currencyConverter::load(const queries &dataSource_)
{
    QSqlQuery q = dataSource_.select(queries::table_Currency, queries::portfolioCurrencyColumns);

    while(q.next())
    {
		int date = q.value(queries::currencyColumns_date).toInt();
		PriceData priceData;
        QMap<currencyType,QMap<currencyType, PriceData> > & curr_to_curr = dateToCurrencyMap[date];
		priceData.price = q.value(queries::currencyColumns_currency_value).toDouble();
		priceData.isTemp = false;

		currencyType from = (currencyType)q.value(queries::currencyColumns_currency_from).toInt();
		currencyType to = (currencyType)q.value(queries::currencyColumns_currency_to).toInt();

        curr_to_curr[from][to] = priceData;
            
    }
}

currencyType currencyConverter::getCurrencyTypeFromSign( const QString& displaySign )
{
    if (0 == displaySign.compare(getDisplaySign(currencyNIS)))
    {
        return currencyNIS;
    }
    else if (0 == displaySign.compare(getDisplaySign(currencyEUR)))
    {
        return currencyEUR;
    }
    else if (0 == displaySign.compare(getDisplaySign(currencyUSD)))
    {
        return currencyUSD;
    }
    else{
        return currencyLast;
    }
}
