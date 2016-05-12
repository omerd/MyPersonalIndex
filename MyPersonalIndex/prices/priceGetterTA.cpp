#include "priceGetterTA.h"

#include <QString>
#include <QList>
#include <QByteArray>
#include <QUrl>
#include <QDate>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <historicalPrices.h>

bool priceGetterTA::isValidSymbol(const QString &symbol_) const
{
    return symbol_.contains(".TA");
}

QString priceGetterTA::getCSVAddress(const QString &symbol_, const QDate &beginDate_, const QDate &endDate_, const QString &type_)
{
	return QString("http://127.0.0.1:8015/table.csv?s=%1&a=%2&b=%3&c=%4&d=%5&e=%6&f=%7&g=%8&ignore=.csv").arg(
		symbol_, QString::number(beginDate_.month() - 1), QString::number(beginDate_.day()), QString::number(beginDate_.year()),
		QString::number(endDate_.month() - 1), QString::number(endDate_.day()), QString::number(endDate_.year()), type_);
}


int priceGetterTA::getPrices(const QString &symbol_, historicalPrices priceHistory_, int beginDate_, int endDate_) const
{
	int earliestUpdate = endDate_ + 1;
	QList<QByteArray> lines =
		downloadFile(QUrl(
		getCSVAddress(
		symbol_,
		QDate::fromJulianDay(beginDate_),
		QDate::fromJulianDay(endDate_),
		QString(stockPrices)
		)
		));

	if (lines.count() <= 2)
		return lines.empty() ? -1 : earliestUpdate; // return true if at least the header row came through

	lines.removeFirst();
	lines.removeLast();

	foreach(const QByteArray &s, lines)
	{
		QList<QByteArray> line = s.split(','); // csv

		int date = QDate::fromString(line.at(0), Qt::ISODate).toJulianDay();
		if (priceHistory_.contains(date, historicalPrices::type_price))
			continue;

		earliestUpdate = qMin(earliestUpdate, date);
		priceHistory_.insert(date, line.at(4).toDouble(), historicalPrices::type_price);
	}

	return earliestUpdate;
}

int priceGetterTA::getDividends(const QString &/*symbol_*/, historicalPrices /*priceHistory_*/, int /*beginDate_*/, int endDate_) const
{
    return ++endDate_; //no-op
}

int priceGetterTA::getSplits(const QString &/*symbol_*/, historicalPrices /*priceHistory_*/, int /*beginDate_*/, int endDate_) const
{
    return ++endDate_;  //no-op
}


QList<QByteArray> priceGetterTA::downloadFile(const QUrl &url_, bool splitResultByLineBreak_)
{
	//http://lists.trolltech.com/qt-interest/2007-11/thread00759-0.html

	QNetworkAccessManager manager;
	QEventLoop loop;
	QNetworkRequest request(url_);
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
