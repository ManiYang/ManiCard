#ifndef NEO4JHTTPAPICLIENT_H
#define NEO4JHTTPAPICLIENT_H

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVector>

class Neo4jTransaction;

//!
//! This class is meant to has one instance, as a long-lived service.
//!
class Neo4jHttpApiClient : public QObject
{
    Q_OBJECT
public: // types
    struct QueryStatement
    {
        QString cypher;
        QJsonObject parameters;
    };

    class QueryResult
    {
    public:
        int rowCount() const;
        bool isEmpty() const;

        //!
        //! \return (value, meta) at (\e row, \e column).
        //!         Returns (\c Undefined, \c Undefined) if not found.
        //!
        std::pair<QJsonValue, QJsonValue> valueAndMetaAt(
                const int row, const int column) const;
        std::pair<QJsonValue, QJsonValue> valueAndMetaAt(
                const int row, const QString &columnName) const;

        //!
        //! \return \c Undefined if not found.
        //!
        QJsonValue valueAt(const int row, const int column) const;
        QJsonValue valueAt(const int row, const QString &columnName) const;

        //

        //!
        //! Parses result object in API response.
        //!
        static QueryResult fromApiResponse(const QJsonObject &resultObject);

    private:
        struct Row
        {
            QVector<QJsonValue> values; // [c]: for column c
            QVector<QJsonValue> metas; // [c]: for column c
        };

        QHash<QString, int> columnNameToIndex;
        QVector<Row> rows;
    };

    struct DbError
    {
        QString code;
        QString message;
    };

    class QueryResponseBase
    {
    public:
        bool hasNetworkError {false};
        QVector<DbError> dbErrors;
        bool hasNetworkOrDbError() const { return hasNetworkError || !dbErrors.isEmpty(); }

    protected:
        QueryResponseBase() {}
        QueryResponseBase(
                const bool hasNetworkError_, const QVector<DbError> &dbErrors_,
                const QVector<QueryResult> &results_)
            : hasNetworkError(hasNetworkError_), dbErrors(dbErrors_), results(results_) {}

        QVector<QueryResult> results; // [i]: result of ith query statement
    };

    struct QueryResponse : public QueryResponseBase
    {
        QueryResponse() : QueryResponseBase() {}
        QueryResponse(
                const bool hasNetworkError_, const QVector<DbError> &dbErrors_,
                const QVector<QueryResult> &results_);

        QVector<QueryResult> getResults() const;
    };

    struct QueryResponseSingleResult : public QueryResponseBase
    {
        QueryResponseSingleResult() : QueryResponseBase() {}

        //!
        //! Elements of \e results_ at index > 0 (if any) are ignored
        //!
        QueryResponseSingleResult(
                const bool hasNetworkError_, const QVector<DbError> &dbErrors_,
                const QVector<QueryResult> &results_);

        std::optional<QueryResult> getResult() const;
    };

public:
    //!
    //! \param dbHostUrl_
    //! \param dbName_
    //! \param dbAuthFilePath_: a text file with username as 1st line and password as 2nd line
    //! \param networkAccessManager_
    //!
    explicit Neo4jHttpApiClient(
            const QString &dbHostUrl_, const QString &dbName_,
            const QString &dbAuthFilePath_, QNetworkAccessManager *networkAccessManager_,
            QObject *parent = nullptr);

    //!
    //! The sequence of queries is wrapped in an implicit transaction. (Use \e INeo4jTransaction
    //! for explicit transactions.)
    //! The request has no time-out and is not retried if there's network error.
    //!
    void queryDb(
            const QVector<QueryStatement> &queryStatements,
            std::function<void (const QueryResponse &response)> callback,
            QPointer<QObject> callbackContext);

    //!
    //! The query is wrapped in an implicit transaction. (Use \e INeo4jTransaction for explicit
    //! transactions.)
    //! The request has no time-out and is not retried if there's network error.
    //!
    void queryDb(
            const QueryStatement &queryStatements,
            std::function<void (const QueryResponseSingleResult &response)> callback,
            QPointer<QObject> callbackContext);

    //!
    //! \return The returned transaction is not yet opened, and has no parent QObject.
    //!
    Neo4jTransaction *getTransaction();

private:
    const QString hostUrl;
    const QString dbName;
    const QString dbAuthFilePath;
    QNetworkAccessManager *networkAccessManager;
};

//!
//! An instance of this class can be obtained with \c Neo4jTransaction::getTransaction() .
//!
//! Although the transaction is kept alive (before commit/rollback), it is better not to have long
//! periods of inactivity (by, e.g., having long-running computations) between queries.
//! If a query results in an error, the transaction is rolled back.
//!
//! All HTTP requests have no time-out mechanism, and are not retried if there's network error.
//!
class Neo4jTransaction : public QObject
{
    Q_OBJECT
public:
    explicit Neo4jTransaction(
            const QString &dbHostUrl_, const QString &dbName_,
            const QString &dbAuthFilePath_, QNetworkAccessManager *networkAccessManager_,
            QObject *parent = nullptr);

    void open(std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    using QueryStatement = Neo4jHttpApiClient::QueryStatement;
    using QueryResponse = Neo4jHttpApiClient::QueryResponse;
    using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult;

    //!
    //! \param statements
    //! \param callback: argument \e response may not contain all errors that occurred
    //! \param callbackContext
    //!
    void query(
            const QVector<QueryStatement> &queryStatements,
            std::function<void (bool ok, const QueryResponse &response)> callback,
            QPointer<QObject> callbackContext);

    //!
    //! \param queryStatement
    //! \param callback: argument \e response may not contain all errors that occurred
    //! \param callbackContext
    //!
    void query(
            const QueryStatement &queryStatement,
            std::function<void (bool ok, const QueryResponseSingleResult &response)> callback,
            QPointer<QObject> callbackContext);

    void commit(std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void rollback(std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    //!
    //! \return false if the transaction is
    //!           + not yet opened,
    //!           + commited,
    //!           + rolled back (explicitly or because of error), or
    //!           + awaiting server response
    //!
    bool canQuery() const;

private:
    const QString hostUrl;
    const QString dbName;
    const QString dbAuthFilePath;
    QNetworkAccessManager *networkAccessManager;

    enum class State {
        NotOpenedYet, Opened, Committed, RolledBack,
        WaitingResponse,
        Error
    };
    State mState {State::NotOpenedYet};

    QString mTransactionId {"na"};
    QTimer *mTimerSendKeepAlive;

    enum class HttpMethod {Post, Delete};

    //!
    //! Also sets \c mState to \c WaitingResponse, and connects the signal
    //! \c QNetworkReply::sslErrors() .
    //!
    QNetworkReply *sendRequest(
            const QString &url, const HttpMethod method, const QByteArray &postBody);

    static QString stateDescription(const State s);
};

#endif // NEO4JHTTPAPICLIENT_H
