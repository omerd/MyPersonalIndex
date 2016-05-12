#ifndef HISTORICALPRICES_H
#define HISTORICALPRICES_H

#include <QSharedData>
#include <QHash>
#include <QString>

class queries;
class historicalPricesData;
class historicalPrices
{
public:
    historicalPrices();
    historicalPrices(const QString &symbol_);
    historicalPrices(const historicalPrices &other_);
    ~historicalPrices();

    historicalPrices& operator=(const historicalPrices &other_);

    enum type {
        type_price,
        type_dividend,
        type_split
    };

    double priceForMissingDate(int date_) const;
    double value(int date_, type type_) const;
    QMap<int, double> values(type type_) const;

    QString symbol() const;
    void setSymbol(const QString &symbol_);

    bool contains(int date_, type type_) const;

    int endDate(type type_) const;
    int beginDate(type type_) const;

    void insert(int date_, double value_, type type_);

    bool insertBatch(const queries &dataSource_);

	void addDateOfTempPrice(int date);
	void removeTempPrices();
private:
    QExplicitlySharedDataPointer<historicalPricesData> d;
};

class historicalPricesMap
{
public:
    historicalPricesMap()
    {}

    historicalPricesMap(const QHash<QString, historicalPrices> &historicalPrices_):
        m_historicalPrices(historicalPrices_)
    {}

    QHash<QString, historicalPrices> getHistoricalPrices() const { return m_historicalPrices; }

    historicalPrices getHistoricalPrice(const QString &symbol_)
    {
        if (!m_historicalPrices.contains(symbol_))
            m_historicalPrices.insert(symbol_, historicalPrices(symbol_));

        return m_historicalPrices[symbol_];
    }

    void clear() { m_historicalPrices.clear(); }
    bool save(const queries &dataSource_);

    bool isEmpty() const { return m_historicalPrices.isEmpty(); }

private:
    QHash<QString, historicalPrices> m_historicalPrices;
};

#endif // HISTORICALPRICES_H
