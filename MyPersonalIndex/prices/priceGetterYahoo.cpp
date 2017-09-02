#include "priceGetterYahoo.h"

#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QEventLoop>
#include <QDate>
#include <QSslConfiguration>
#include "historicalPrices.h"

QString priceGetterYahoo::getCSVAddress(const QString &symbol_, const QDate &beginDate_, const QDate &endDate_, const QString &type_)
{
    return QString("http://127.0.0.1:8017/ichart.finance.yahoo.com/table.csv?s=%1&a=%2&b=%3&c=%4&d=%5&e=%6&f=%7&g=%8&ignore=.csv").arg(
        symbol_, QString::number(beginDate_.month()), QString::number(beginDate_.day()), QString::number(beginDate_.year()),
                QString::number(endDate_.month()), QString::number(endDate_.day()), QString::number(endDate_.year()), type_);
}

QString priceGetterYahoo::getSplitAddress(const QString &symbol, const QDate &beginDate_, const QDate &endDate_)
{
    return QString("http://127.0.0.1:8017/ichart.finance.yahoo.com/x?s=%1&a=%2&b=%3&c=%4&d=%5&e=%6&f=%7&g=v&y=0&z=30000").arg(
		symbol, QString::number(beginDate_.month()), QString::number(beginDate_.day()), QString::number(beginDate_.year()),
		QString::number(endDate_.month()), QString::number(endDate_.day()), QString::number(endDate_.year()));
}

QList<QByteArray> priceGetterYahoo::downloadFile(const QUrl &url_, bool splitResultByLineBreak_)
{
    //http://lists.trolltech.com/qt-interest/2007-11/thread00759-0.html

    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkRequest request;

	QSslConfiguration config = QSslConfiguration::defaultConfiguration();
	config.setProtocol(QSsl::TlsV1);//maybe v1 will become deprecated one day.
	request.setSslConfiguration(config);
	request.setUrl(QUrl(url_));

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    loop.exec();

    QList<QByteArray> lines;
    if (reply->error() == QNetworkReply::NoError)
    {
        if (splitResultByLineBreak_)
            lines = reply->readAll().split('\n');
        else
            lines.append(reply->readAll());
    }

    delete reply;
    return lines;
}

QByteArray priceGetterYahoo::getPricesForToday(const QString &symbol_, historicalPrices priceHistory_) const
{
    QList<QByteArray> lines =
        downloadFile(QUrl( QString("http://download.finance.yahoo.com/d/quotes.csv?s=%1&f=d1ohgl1vl1").arg(symbol_) ));
    if (lines.count() > 1)
    {
        return lines[0];
    }
    else
    {
        return QByteArray();
    }
}

int priceGetterYahoo::getPrices(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const
{
    const int TODAY_DATE = QDate::currentDate().toJulianDay();

    int earliestUpdate = endDate_ + 1;
    QList<QByteArray> lines =
        downloadFile(QUrl(
            getCSVAddress(
                symbol_,
                QDate::fromJulianDay(beginDate_),
                QDate::fromJulianDay(endDate_ + 1),
                QString(stockPrices)
            )
        ));


    QByteArray todayInfo;
    bool todayDataAvailable = false;
    if(TODAY_DATE == endDate_)
    {
        todayInfo = getPricesForToday(symbol_, priceHistory_);
        if(todayInfo.size() > 0)
        {
            todayDataAvailable = true;
        }
    }

    if (lines.count() <= 2)
	{   
		lines.clear();
        if(!todayDataAvailable)
        {
            return lines.empty() ? -1 : earliestUpdate; // return true if at least the header row came through
        }
        else
        {
            lines.push_front("\n");
            lines.push_back(todayInfo);
            lines.push_back("\n");
        }
	}

    lines.removeFirst();
    lines.removeLast();

    foreach(const QByteArray &s, lines)
    {
        QList<QByteArray> line = s.split(','); // csv

		QString s_date = line.at(0);
        int date = QDate::fromString(s_date, Qt::ISODate).toJulianDay();
		if (0 == date)
		{
			s_date.replace("\"",""); //remove "" the encloses the date
            date = QDate::fromString(s_date,"MM/dd/yyyy").toJulianDay();
            QDate tmp = QDate::fromJulianDay(date);

            if (0 == date)
            {
                date = QDate::fromString(s_date,"MM/d/yyyy").toJulianDay();
            }
            if (0 == date)
            {
                date = QDate::fromString(s_date,"M/d/yyyy").toJulianDay();
            }
		}
        if (priceHistory_.contains(date, historicalPrices::type_price))
            continue;

        earliestUpdate = qMin(earliestUpdate, date);
        if(todayDataAvailable && date == TODAY_DATE){
            priceHistory_.addDateOfTempPrice(TODAY_DATE);
		}
        priceHistory_.insert(date, line.at(4).toDouble(), historicalPrices::type_price);
    }

    return earliestUpdate;
}

int priceGetterYahoo::getDividends(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const
{
    int earliestUpdate = endDate_ + 1;
    QList<QByteArray> lines =
        downloadFile(QUrl(
            getCSVAddress(
                symbol_,
                QDate::fromJulianDay(beginDate_),
                QDate::fromJulianDay(endDate_  + 1),
                QString(stockDividends)
            )
        ));

    if (lines.count() <= 2)
        return earliestUpdate;

    lines.removeFirst();
    lines.removeLast();

    foreach(const QByteArray &s, lines)
    {
		QList<QByteArray> line = s.split(','); // csv
		QString type = line.at(1);
		if (-1 == type.indexOf("Dividend", 0, Qt::CaseInsensitive)){
			continue;
		}

		QString dateString = line.at(0);
		int date = QDate::fromString(dateString, Qt::ISODate).toJulianDay();
		if (priceHistory_.contains(date, historicalPrices::type_dividend))
			continue;

		earliestUpdate = qMin(earliestUpdate, date);
		priceHistory_.insert(date, line.at(2).toDouble(), historicalPrices::type_dividend);
    }

    return earliestUpdate;
}

int priceGetterYahoo::getSplits(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const
{
    int earliestUpdate = endDate_ + 1;
    QList<QByteArray> lines = downloadFile(QUrl(getSplitAddress(symbol_, QDate::fromJulianDay(beginDate_),
                                                                QDate::fromJulianDay(endDate_ + 1))), true);

    if (lines.isEmpty())
        return earliestUpdate;
	lines.removeFirst();
	if (lines.isEmpty())
		return earliestUpdate;
	lines.removeLast();

	foreach(const QByteArray &s, lines)
	{
		QList<QByteArray> line = s.split(','); // csv
		QString type = line.at(1);
		if (-1 == type.indexOf("Split", 0, Qt::CaseInsensitive)){
			continue;
		}

        QString dateString = line.at(0);
        int date = QDate::fromString(dateString, Qt::ISODate).toJulianDay();
		
		if (date < beginDate_ || date > endDate_ || priceHistory_.contains(date, historicalPrices::type_split))
			continue;


		earliestUpdate = qMin(earliestUpdate, date);

		if (line.at(2).toDouble() == 0) // just in case
			continue;

		double ratio = line.at(2).toDouble();

		priceHistory_.insert(date, ratio, historicalPrices::type_split);
	}

    return earliestUpdate;
}

bool priceGetterYahoo::isValidSymbol( const QString &symbol_ ) const
{
	return !symbol_.contains(".TA");
}
