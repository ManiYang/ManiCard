#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include "neo4j_http_api_client.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"

namespace {

QString removeSlashAtEnd(const QString &s) {
    if (s.endsWith(QChar('/')))
        return s.chopped(1);
    else
        return s;
}

//!
//! \param authFilePath: a text file with username in 1st line and password in 2nd line
//!
QByteArray getBasicAuthData(const QString &authFilePath) {
    QByteArray user;
    QByteArray password;
    {
        QFile file(authFilePath);
        const bool ok = file.open(QIODevice::ReadOnly);
        if (!ok) {
            qWarning() << QString("could not open file %1 for reading").arg(authFilePath);
            return "";
        }

        user = file.readLine().trimmed();
        password = file.readLine().trimmed();
    }

    return (user + ':' + password).toBase64();
}

void addCommonHeadersToRequest(
        QNetworkRequest &networkRequest, const QByteArray &basicAuthData) {
    networkRequest.setRawHeader("Accept", "application/json;charset=UTF-8");
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkRequest.setRawHeader("Authorization", "Basic " + basicAuthData);
}

QByteArray prepareQueryRequestBody(const QVector<Neo4jHttpApiClient::QueryStatement> &statements) {
    QJsonArray statementsArray;
    for (const auto &statement: statements) {
        statementsArray << QJsonObject {
            {"statement", statement.cypher},
            {"parameters", statement.parameters}
        };
    }

    return QJsonDocument(QJsonObject {{"statements", statementsArray}}).toJson();
}

void logSslErrors(const QList<QSslError> &errors)
{
    QString errMsg = "SSL error(s):";
    for (const QSslError &error: errors)
        errMsg += QString("\n    %1 (code: %2)").arg(error.errorString()).arg(error.error());
    qWarning().noquote() << errMsg;
}

void logDbErrorMessages(const QVector<Neo4jHttpApiClient::DbError> &errors)
{
    qWarning().noquote() << QString("errors from DB:");
    for (const auto &error: errors)
        qWarning().noquote() << QString("  + %1 -- %2").arg(error.code, error.message);
}

Neo4jHttpApiClient::QueryResponse handleApiResponse(
        QNetworkReply *reply, QString *transactionId = nullptr) {
    if (reply->error() != QNetworkReply::NoError) {
        const auto msg
                = QString("Network error while sending request to %1 -- %2")
                  .arg(reply->request().url().toString(), reply->errorString());
        qWarning().noquote() << msg;
        return Neo4jHttpApiClient::QueryResponse(true, {}, {});
    }

    //
    const QByteArray replyBody = reply->readAll();

    const auto replyObject = QJsonDocument::fromJson(replyBody).object();
    const auto resultsArray = replyObject.value("results").toArray();
    const auto errorsArray = replyObject.value("errors").toArray();
    const QString commitUrl = replyObject.value("commit").toString();

    //
    Neo4jHttpApiClient::QueryResponse queryResponse(false, {}, {});

    for (const QJsonValue &result: resultsArray)
        queryResponse.results << Neo4jHttpApiClient::QueryResult::fromApiResponse(result);

    for (const QJsonValue &error: errorsArray) {
        const auto errorObject = error.toObject();
        queryResponse.dbErrors << Neo4jHttpApiClient::DbError {
                errorObject.value("code").toString(),
                errorObject.value("message").toString()
        };
    }

    if (transactionId != nullptr) {
        *transactionId = "";
        if (!commitUrl.isEmpty()) {
            // get transaction ID from `commitUrl`
            // `commitUrl` is like "http://localhost:<port>/db/<db_name>/tx/<transaction_id>/commit"
            static QRegularExpression re {R"(.*/db/[^/]+/tx/(\d+)/commit$)"};
            auto m = re.match(commitUrl);
            if (m.hasMatch())
                *transactionId = m.captured(1);
            else
                qWarning().noquote() << "failed to extract transaction ID from response body";
        }
    }

    return queryResponse;
}

} // namespace

//====

Neo4jHttpApiClient::Neo4jHttpApiClient(const QString &dbHostUrl_, const QString &dbName_,
        const QString &dbAuthFilePath_, QNetworkAccessManager *networkAccessManager_,
        QObject *parent)
    : QObject(parent)
    , hostUrl(removeSlashAtEnd(dbHostUrl_))
    , dbName(dbName_)
    , dbAuthFilePath(dbAuthFilePath_)
    , networkAccessManager(networkAccessManager_)
{}

void Neo4jHttpApiClient::queryDb(const QVector<QueryStatement> &queryStatements,
        std::function<void (const QueryResponse &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    QNetworkRequest request;
    request.setUrl(QUrl(QString("%1/db/%2/tx/commit").arg(hostUrl, dbName)));
    addCommonHeadersToRequest(request, getBasicAuthData(dbAuthFilePath));

    //
    QNetworkReply *reply
            = networkAccessManager->post(request, prepareQueryRequestBody(queryStatements));

    connect(reply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &errors) {
        logSslErrors(errors);
    });

    connect(reply, &QNetworkReply::finished, this, [reply, callback, callbackContext]() {
        const QueryResponse queryResponse = handleApiResponse(reply);
        reply->deleteLater();

        invokeAction(callbackContext, [queryResponse, callback]() {
            callback(queryResponse);
        });

    });
}

Neo4jTransaction *Neo4jHttpApiClient::getTransaction()
{
    return new Neo4jTransaction(hostUrl, dbName, dbAuthFilePath, networkAccessManager, nullptr);
}

//====

Neo4jTransaction::Neo4jTransaction(
        const QString &dbHostUrl_, const QString &dbName_, const QString &dbAuthFilePath_,
        QNetworkAccessManager *networkAccessManager_, QObject *parent)
    : QObject(parent)
    , hostUrl(removeSlashAtEnd(dbHostUrl_))
    , dbName(dbName_)
    , dbAuthFilePath(dbAuthFilePath_)
    , networkAccessManager(networkAccessManager_)
    , mTimerSendKeepAlive(new QTimer(this))
{
    // set up `mTimerSendKeepAlive`
    constexpr int intervalSendKeepAliveQuery = 1500;
    mTimerSendKeepAlive->setInterval(intervalSendKeepAliveQuery);
    connect(mTimerSendKeepAlive, &QTimer::timeout, this, [this]() {
        if (mState != State::Opened)
            return;
        // send empty query for keep-alive
        query(
                {},
                [](bool /*ok*/, const QueryResponse &/*response*/) {},
                this
        );
        qInfo().noquote()
                << QString("sent keep-alive query for transaction ID %1").arg(mTransactionId);
    });
}

void Neo4jTransaction::open(
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    std::optional<bool> earlyResult;
    switch (mState) {
    case State::NotOpenedYet:
        break;

    case State::Opened:
        earlyResult = true;
        break;

    case State::Committed: [[fallthrough]];
    case State::RolledBack: [[fallthrough]];
    case State::WaitingResponse: [[fallthrough]];
    case State::Error:
        qWarning().noquote() << stateDescription(mState);
        earlyResult = false;
        break;
    }

    if (earlyResult.has_value()) {
        invokeAction(callbackContext, [callback, ok=earlyResult.value()]() {
            callback(ok);
        });
        return;
    }

    //
    QNetworkReply *reply = sendRequest(
            QString("%1/db/%2/tx").arg(hostUrl, dbName),
            HttpMethod::Post,
            QByteArray()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, callbackContext]() {
        const QueryResponse queryResponse = handleApiResponse(reply, &mTransactionId);
        reply->deleteLater();

        bool openOk = true;
        if (queryResponse.hasNetworkError) {
            openOk = false;
        }
        else if (!queryResponse.dbErrors.isEmpty()) {
            logDbErrorMessages(queryResponse.dbErrors);
            openOk = false;
        }
        else if (mTransactionId.isEmpty()) {
            openOk = false;
        }

        // update state
        if (openOk) {
            mState = State::Opened;
            mTimerSendKeepAlive->start();
            qInfo().noquote() << QString("opened transaction ID %1").arg(mTransactionId);
        }
        else {
            mState = State::Error;
        }

        // invoke `callback`
        invokeAction(callbackContext, [openOk, callback]() {
            callback(openOk);
        });
    });
}

void Neo4jTransaction::query(
        const QVector<QueryStatement> &queryStatements,
        std::function<void (bool, const QueryResponse &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    if (mState != State::Opened) {
        qWarning().noquote() << stateDescription(mState);
        invokeAction(callbackContext, [callback]() {
            callback(false, {});
        });
        return;
    }

    //
    QNetworkReply *reply = sendRequest(
            QString("%1/db/%2/tx/%3").arg(hostUrl, dbName, mTransactionId),
            HttpMethod::Post,
            prepareQueryRequestBody(queryStatements)
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, callbackContext]() {
        QString transactionId;
        const QueryResponse queryResponse = handleApiResponse(reply, &transactionId);
        reply->deleteLater();

        bool requestOk = true;
        if (queryResponse.hasNetworkError) {
            requestOk = false;
        }
        else if (!queryResponse.dbErrors.isEmpty()) {
            logDbErrorMessages(queryResponse.dbErrors);
            requestOk = false;
        }
        else if (transactionId.isEmpty()) {
            requestOk = false;
        }

        // update state
        if (!requestOk) {
            mState = State::Error;
            mTimerSendKeepAlive->stop();
        }
        else {
            mTimerSendKeepAlive->start();
        }

        // call `callback`
        invokeAction(callbackContext, [requestOk, callback, queryResponse]() {
            callback(requestOk, queryResponse);
        });
    });
}

void Neo4jTransaction::commit(
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    if (mState != State::Opened) {
        qWarning().noquote() << stateDescription(mState);
        invokeAction(callbackContext, [callback]() {
            callback(false);
        });
        return;
    }

    //
    QNetworkReply *reply = sendRequest(
            QString("%1/db/%2/tx/%3/commit").arg(hostUrl, dbName, mTransactionId),
            HttpMethod::Post,
            QByteArray()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, callbackContext]() {
        const QueryResponse queryResponse = handleApiResponse(reply);
        reply->deleteLater();

        bool commitOk = true;
        if (queryResponse.hasNetworkError) {
            commitOk = false;
        }
        else if (!queryResponse.dbErrors.isEmpty()) {
            logDbErrorMessages(queryResponse.dbErrors);
            commitOk = false;
        }

        // update state
        if (commitOk) {
            mState = State::Committed;
            qInfo().noquote() << QString("committed transaction ID %1").arg(mTransactionId);
        }
        else {
            mState = State::Error;
        }

        mTimerSendKeepAlive->stop();

        // call `callback`
        invokeAction(callbackContext, [commitOk, callback]() {
            callback(commitOk);
        });
    });
}

void Neo4jTransaction::rollback(
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    if (mState != State::Opened) {
        qWarning().noquote() << stateDescription(mState);
        invokeAction(callbackContext, [callback]() {
            callback(false);
        });
        return;
    }

    //
    QNetworkReply *reply = sendRequest(
            QString("%1/db/%2/tx/%3").arg(hostUrl, dbName, mTransactionId),
            HttpMethod::Delete,
            QByteArray()
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, callbackContext]() {
        const QueryResponse queryResponse = handleApiResponse(reply);
        reply->deleteLater();

        bool commitOk = true;
        if (queryResponse.hasNetworkError) {
            commitOk = false;
        }
        else if (!queryResponse.dbErrors.isEmpty()) {
            logDbErrorMessages(queryResponse.dbErrors);
            commitOk = false;
        }

        // update state
        if (commitOk)
            mState = State::RolledBack;
        else
            mState = State::Error;

        mTimerSendKeepAlive->stop();

        // invoke `callback`
        invokeAction(callbackContext, [commitOk, callback]() {
            callback(commitOk);
        });
    });
}

bool Neo4jTransaction::isOpened() const {
    return mState == State::Opened;
}

QNetworkReply *Neo4jTransaction::sendRequest(
        const QString &url, const HttpMethod method, const QByteArray &postBody)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    addCommonHeadersToRequest(request, getBasicAuthData(dbAuthFilePath));

    //
    QNetworkReply *reply = nullptr;
    switch (method) {
    case HttpMethod::Post:
        reply = networkAccessManager->post(request, postBody);
        break;
    case HttpMethod::Delete:
        reply = networkAccessManager->deleteResource(request);
        break;
    }
    Q_ASSERT(reply != nullptr);

    mState = State::WaitingResponse;

    //
    connect(reply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &errors) {
        logSslErrors(errors);
    });

    //
    return reply;
}

QString Neo4jTransaction::stateDescription(const State s)
{
    switch (s) {
    case State::NotOpenedYet:
        return "transaction is not opened yet";

    case State::Opened:
        return "transaction is already opened";

    case State::Committed:
        return "transaction has been committed";

    case State::RolledBack:
        return "transaction has been explicitly rolled back";

    case State::WaitingResponse:
        return "waiting for server response";

    case State::Error:
        return "transaction had encountered error";
    }
    return "";
}

//====

int Neo4jHttpApiClient::QueryResult::rowCount() const {
    return rows.count();
}

bool Neo4jHttpApiClient::QueryResult::isEmpty() const {
    return rows.isEmpty();
}

std::pair<QJsonValue, QJsonValue> Neo4jHttpApiClient::QueryResult::valueAndMetaAt(
        const int row, const int column) const
{
    if (row < 0 || row >= rows.count())
        return {QJsonValue::Undefined, QJsonValue::Undefined};

    return {
        rows.at(row).values.value(column, QJsonValue::Undefined),
        rows.at(row).metas.value(column, QJsonValue::Undefined)
    };
}

std::pair<QJsonValue, QJsonValue> Neo4jHttpApiClient::QueryResult::valueAndMetaAt(
        const int row, const QString &columnName) const
{
    if (!columnNameToIndex.contains(columnName))
        return {QJsonValue::Undefined, QJsonValue::Undefined};
    return valueAndMetaAt(row, columnNameToIndex.value(columnName));
}

QJsonValue Neo4jHttpApiClient::QueryResult::valueAt(const int row, const int column) const
{
    if (row < 0 || row >= rows.count())
        return QJsonValue::Undefined;
    return rows.at(row).values.value(column, QJsonValue::Undefined);
}

QJsonValue Neo4jHttpApiClient::QueryResult::valueAt(const int row, const QString &columnName)
{
    if (!columnNameToIndex.contains(columnName))
        return QJsonValue::Undefined;
    return valueAt(row, columnNameToIndex.value(columnName));
}

Neo4jHttpApiClient::QueryResult Neo4jHttpApiClient::QueryResult::fromApiResponse(
        const QJsonValue &resultJsonValue) {
    QueryResult queryResult;

    const auto resultObj = resultJsonValue.toObject();

    const auto columnsArray = resultObj.value("columns").toArray();
    for (int i = 0; i < columnsArray.count(); ++i)
        queryResult.columnNameToIndex.insert(columnsArray.at(i).toString(), i);

    const auto recordsArray = resultObj.value("data").toArray();
    queryResult.rows.reserve(recordsArray.count());
    for (const QJsonValue &record: recordsArray)
    {
        const auto recordObj = record.toObject();

        const auto valueArray = recordObj.value("row").toArray(); // values of columns
        const auto metaArray = recordObj.value("meta").toArray(); // meta's of columns
        if (metaArray.count() < valueArray.count())
        {
            qWarning().noquote() << "array `meta` has fewer elements than array `row`";
            qWarning().noquote() << QString("  | the result is: %1").arg(printJson(resultObj));
        }

        Row row;
        row.values.reserve(valueArray.count());
        row.metas.reserve(valueArray.count());
        for (int i = 0; i < valueArray.count(); ++i)
        {
            row.values.append(valueArray.at(i));
            row.metas.append(
                    (i < metaArray.count()) ? metaArray.at(i) : QJsonValue(QJsonValue::Null)
            );
        }

        queryResult.rows << row;
    }

    return queryResult;
}
