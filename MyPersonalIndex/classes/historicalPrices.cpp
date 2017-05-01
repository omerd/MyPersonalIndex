#include "historicalPrices.h"
#include <QMap>
#include <QString>
#include <QVariant>
#include <Qdate>
#include <Qlist>
#include "queries.h"
#include "queriesBatch.h"

class historicalPricesData: public QSharedData, public queriesBatch
{
public:
    QMap<int, double> splits;
    QMap<int, double> dividends;
    QMap<int, double> prices;
    QString symbol;
	QList<int> temporaryPrices;

    historicalPricesData()
    {}

    historicalPricesData(const QString &symbol_):
        symbol(symbol_)
    {}

    void insert(int date_, double value_, historicalPrices::type type_)
    {
        switch(type_)
        {
            case historicalPrices::type_price:
                prices.insert(date_, value_);
                break;
            case historicalPrices::type_dividend:
                dividends.insert(date_, value_);
                break;
            case historicalPrices::type_split:
                splits.insert(date_, value_);
                break;
        };
    }

    bool insertBatch(const queries &dataSource_)
    {
        if (symbol.isEmpty())
            return false;

        m_position = prices.constBegin();
        m_positionType = historicalPrices::type_price;
        return dataSource_.bulkInsert(queries::table_HistoricalPrice, queries::historicalPriceColumns, prices.count() + dividends.count() + splits.count(), this);
    }

    QVariant data(int column_, bool newRow_)
    {
        if (newRow_)
            ++m_position;

        if (m_position == prices.constEnd())
        {
            m_position = dividends.constBegin();
            m_positionType = historicalPrices::type_dividend;
        }

        if (m_position == dividends.constEnd())
        {
            m_position = splits.constBegin();
            m_positionType = historicalPrices::type_split;
        }

        switch(column_)
        {
            case queries::historicalPriceColumns_Date:
                return m_position.key();
            case queries::historicalPriceColumns_Symbol:
                return symbol;
            case queries::historicalPriceColumns_Type:
                return (int)m_positionType;
            case queries::historicalPriceColumns_Value:
                return m_position.value();
        }
        return QVariant();
    }

private:
    QMap<int, double>::const_iterator m_position;
    historicalPrices::type m_positionType;
};

historicalPrices::historicalPrices():
    d(new historicalPricesData())
{
}

historicalPrices::historicalPrices(const QString &symbol_):
    d(new historicalPricesData(symbol_))
{
}

historicalPrices::historicalPrices(const historicalPrices &other_):
    d(other_.d)
{
}

historicalPrices::~historicalPrices()
{
}

historicalPrices& historicalPrices::operator=(const historicalPrices &other_)
{
    d = other_.d;
    return *this;
}

double historicalPrices::priceForMissingDate(int date_) const
{
    //TODO: binary search, but it's not simple since there are many holes in the list.
    int endDate = (d->prices.constEnd()-1).key();
	int beginDate = (d->prices.constBegin()).key();
    if(0 == endDate || date_ < beginDate)
    {
        return 0;
    }
#ifdef _DEBUG
	QString f2d = QDate::fromJulianDay(date_).toString("yyyy-MM-dd");
#endif
    if(date_ < endDate && date_ != 0)
    {
        while(date_ <= endDate)
        {
            if(d->prices.contains(date_))
            {
                return d->prices.value(date_, 0);
            }
            date_++;
        }
    }
    else
    {
        while(date_ >= endDate)
        {
            if(d->prices.contains(date_))
            {
                return d->prices.value(date_, 0);
            }
            date_--;
        }
    }


    return 0;
}

double historicalPrices::value(int date_, type type_) const
{
    switch(type_)
    {
        case type_price:
            if (contains(date_, type_))
                return d->prices.value(date_, 0);
            else
                return priceForMissingDate(date_);
        case type_dividend:
            return d->dividends.value(date_, 0);
        case type_split:
            return d->splits.value(date_, 1);
    }
    return 0;
}

QMap<int, double> historicalPrices::values(type type_) const
{
    switch(type_)
    {
        case type_price:
            return d->prices;
        case type_dividend:
            return d->dividends;
        case type_split:
            return d->splits;
    }
    return QMap<int, double>();
}

bool historicalPrices::contains(int date_, type type_) const
{
    switch(type_)
    {
        case type_price:
            return d->prices.contains(date_);
        case type_dividend:
            return d->dividends.contains(date_);
        case type_split:
            return d->splits.contains(date_);
    }
    return false;
}

void historicalPrices::insert(int date_, double value_, type type_)
{
    d->insert(date_, value_, type_);
}

bool historicalPrices::insertBatch(const queries &dataSource_)
{
    return d->insertBatch(dataSource_);
}

int historicalPrices::endDate(type type_) const
{
    switch(type_)
    {
        case type_price:
            return d->prices.isEmpty() ? 0 : (d->prices.constEnd() - 1).key();
        case type_dividend:
            return d->dividends.isEmpty() ? 0 : (d->dividends.constEnd() - 1).key();
        case type_split:
            return d->splits.isEmpty() ? 0 : (d->splits.constEnd() - 1).key();
    }
    return 0;
}

int historicalPrices::beginDate(type type_) const
{
    switch(type_)
    {
        case type_price:
            return d->prices.isEmpty() ? 0 : (d->prices.constBegin()).key();
        case type_dividend:
            return d->dividends.isEmpty() ? 0 : (d->dividends.constBegin()).key();
        case type_split:
            return d->splits.isEmpty() ? 0 : (d->splits.constBegin()).key();
    }
    return 0;
}

QString historicalPrices::symbol() const
{
    return d->symbol;
}

void historicalPrices::setSymbol(const QString &symbol_)
{
    d->symbol = symbol_;
}

void historicalPrices::removeTempPrices()
{
	for(QList<int>::iterator i = d->temporaryPrices.begin(); i != d->temporaryPrices.end(); i++)
	//int dateToday = QDate::currentDate().toJulianDay();
	//if(endDate(type_price) == dateToday)
	{
		d->prices.remove(*i);
	}
	d->temporaryPrices.clear();
}

void historicalPrices::addDateOfTempPrice( int date )
{
	d->temporaryPrices.append(date);
}

bool historicalPricesMap::save(const queries &dataSource_)
{
    if (!dataSource_.deleteTable(queries::table_HistoricalPrice))
        return false;

    for(QHash<QString, historicalPrices>::iterator i = m_historicalPrices.begin(); i != m_historicalPrices.end(); i++)
	{
		i->removeTempPrices();
        if (!i->insertBatch(dataSource_))
			return false;
	}

    return true;
}
