#ifndef QUERIES_H
#define QUERIES_H

#include <QSqlDatabase>
#include <QStringList>

template <class T, class V>
class QMap;
class QSqlQuery;
class queriesBatch;
class queries
{
public:
    static const QString table_HistoricalPrice;
	static const QString table_Currency;
    static const QString table_Portfolio;
    static const QString table_PortfolioAA;
    static const QString table_PortfolioAccount;
    static const QString table_PortfolioSecurity;
    static const QString table_PortfolioSecurityAA;
    static const QString table_PortfolioSecurityTrade;
    static const QString table_PortfolioSecurityTradeExecution;

    static const QString view_PortfolioSecurityAA;
    static const QString view_PortfolioSecurityTrade;
    static const QString view_PortfolioSecurityTradeExecution;

    static const QStringList historicalPriceColumns;
	static const QStringList portfolioCurrencyColumns;
    static const QStringList portfolioColumns;
    static const QStringList portfolioAAColumns;
    static const QStringList portfolioAccountColumns;
    static const QStringList portfolioSecurityColumns;
    static const QStringList portfolioSecurityAAColumns;
    static const QStringList portfolioSecurityTradeColumns;
    static const QStringList portfolioSecurityTradeExecutionColumns;

    static const QStringList portfolioSecurityAAViewColumns;
    static const QStringList portfolioSecurityTradeViewColumns;
    static const QStringList portfolioSecurityTradeExecutionViewColumns;

    // NOTE: when changing these enums, modify the corresponding table's QStringList in the cpp
    enum {
        portfolioAAColumns_ID,
        portfolioAAColumns_PortfolioID,
        portfolioAAColumns_Description,
        portfolioAAColumns_Target,
        portfolioAAColumns_RebalanceBand,
        portfolioAAColumns_Threshold,
        portfolioAAColumns_Hide
    };

    enum {
        portfolioAccountColumns_ID,
        portfolioAccountColumns_PortfolioID,
        portfolioAccountColumns_Description,
        portfolioAccountColumns_TaxRate,
        portfolioAccountColumns_TaxDeferred,
        portfolioAccountColumns_CostBasis,
        portfolioAccountColumns_Hide
    };

    enum {
        historicalPriceColumns_Date,
        historicalPriceColumns_Symbol,
        historicalPriceColumns_Type,
        historicalPriceColumns_Value
    };

	enum {
		currencyColumns_date,
		currencyColumns_currency_from,
		currencyColumns_currency_to,
		currencyColumns_currency_value
	};

    enum {
        portfolioColumns_ID,
        portfolioColumns_Description,
        portfolioColumns_StartDate
    };

    enum {
        portfolioSecurityColumns_ID,
        portfolioSecurityColumns_PortfolioID,
        portfolioSecurityColumns_Symbol,
        portfolioSecurityColumns_Account,
		portfolioSecurityColumns_Currency,
        portfolioSecurityColumns_Expense,
        portfolioSecurityColumns_DivReinvest,
        portfolioSecurityColumns_CashAccount,
        portfolioSecurityColumns_IncludeInCalc,
        portfolioSecurityColumns_Hide,
        portfolioSecurityColumns_Note,
        portfolioSecurityColumns_Dividends
    };

    enum {
        portfolioSecurityAAColumns_SecurityID,
        portfolioSecurityAAColumns_AAID,
        portfolioSecurityAAColumns_Percent
    };

    enum {
        portfolioSecurityTradeColumns_ID,
        portfolioSecurityTradeColumns_SecurityID,
        portfolioSecurityTradeColumns_Type,
        portfolioSecurityTradeColumns_Value,
        portfolioSecurityTradeColumns_Price,
        portfolioSecurityTradeColumns_Commission,
        portfolioSecurityTradeColumns_CashAccountID,
        portfolioSecurityTradeColumns_Frequency,
        portfolioSecurityTradeColumns_Date,
        portfolioSecurityTradeColumns_StartDate,
        portfolioSecurityTradeColumns_EndDate,
        portfolioSecurityTradeColumns_Description,
        portfolioSecurityTradeColumns_PriceType
    };

    enum {
        portfolioSecurityTradeExecutionColumns_SecurityID,
        portfolioSecurityTradeExecutionColumns_Date,
        portfolioSecurityTradeExecutionColumns_Shares,
        portfolioSecurityTradeExecutionColumns_Price,
        portfolioSecurityTradeExecutionColumns_Commission,
        portfolioSecurityTradeExecutionColumns_AssociatedTradeID
    };

    enum {
        portfolioSecurityTradeExecutionViewColumns_SecurityID,
        portfolioSecurityTradeExecutionViewColumns_Date,
        portfolioSecurityTradeExecutionViewColumns_Shares,
        portfolioSecurityTradeExecutionViewColumns_Price,
        portfolioSecurityTradeExecutionViewColumns_Commission,
        portfolioSecurityTradeExecutionViewColumns_AssociatedTradeID,
        portfolioSecurityTradeExecutionViewColumns_PortfolioID
    };

    enum {
        portfolioSecurityAAViewColumns_SecurityID,
        portfolioSecurityAAViewColumns_AAID,
        portfolioSecurityAAViewColumns_Percent,
        portfolioSecurityAAViewColumns_PortfolioID
    };

    enum {
        portfolioSecurityTradeViewColumns_ID,
        portfolioSecurityTradeViewColumns_SecurityID,
        portfolioSecurityTradeViewColumns_Type,
        portfolioSecurityTradeViewColumns_Value,
        portfolioSecurityTradeViewColumns_Price,
        portfolioSecurityTradeViewColumns_Commission,
        portfolioSecurityTradeViewColumns_CashAccountID,
        portfolioSecurityTradeViewColumns_Frequency,
        portfolioSecurityTradeViewColumns_Date,
        portfolioSecurityTradeViewColumns_StartDate,
        portfolioSecurityTradeViewColumns_EndDate,
        portfolioSecurityTradeViewColumns_Description,
        portfolioSecurityTradeViewColumns_PriceType,
        portfolioSecurityTradeViewColumns_PortfolioID
    };

    queries(const QString &databaseLocation_);
    void close();

    bool bulkInsert(const QString &tableName_, const QStringList &columns_, int rowCount_, queriesBatch* const object_) const;
    bool insert(const QString &tableName_, const QMap<QString, QVariant> &values_) const;
    QSqlQuery select(const QString &tableName_, const QStringList &columns_) const;
    bool executeNonQuery(const QString &query_) const;
    bool deleteTable(const QString &table_) const;

    void beginTransaction() { m_database.transaction(); }
    void commit() { m_database.commit(); }
    void rollback() { m_database.rollback(); }

    int getDatabaseVersion() const;
    QString getDatabaseLocation() const { return m_database.databaseName(); }
    bool isValid() const;

private:
    QSqlDatabase m_database;
};

#endif // QUERIES_H
