#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QTest>
#include "neo4j_http_api_client.h"
#include "test_util.h"
#include "utilities/json_util.h"

class Neo4jHttpApiClientTest : public testing::Test
{
protected:
    static void SetUpTestSuite() {
        app = QCoreApplication::instance();
        ASSERT_TRUE(app != nullptr);

        // read config file
        const QString configFilePath = QFileInfo(__FILE__).absoluteDir().filePath("config.json");
        const QJsonObject config = readJsonFile(configFilePath).object();
        ASSERT_TRUE(!config.isEmpty());

        //
        networkAccessManager = new QNetworkAccessManager(app);

        const auto neo4jDbInfo = std::make_tuple(
                JsonReader(config)["neo4j_db"]["http_url"].getString(),
                JsonReader(config)["neo4j_db"]["database"].getString(),
                JsonReader(config)["neo4j_db"]["auth_file"].getString());
        ASSERT_TRUE(!std::get<0>(neo4jDbInfo).isEmpty());
        ASSERT_TRUE(!std::get<1>(neo4jDbInfo).isEmpty());
        ASSERT_TRUE(!std::get<2>(neo4jDbInfo).isEmpty());

        neo4jHttpApiClient = new Neo4jHttpApiClient(
                std::get<0>(neo4jDbInfo), std::get<1>(neo4jDbInfo), std::get<2>(neo4jDbInfo),
                networkAccessManager, qApp);
    }

    static void TearDownTestSuite() {}

    //
    static void openTransaction(Neo4jTransaction *transaction) {
        std::optional<bool> openOk;
        transaction->open(
                [&openOk](bool ok) { openOk = ok; },
                app
        );
        const bool ok = waitUntilHasValue(openOk);
        ASSERT_TRUE(ok) << "no response from open() within timeout";
        ASSERT_TRUE(openOk.value());
    }

    //
    inline static QCoreApplication *app = nullptr; // just for convenience
    inline static Neo4jHttpApiClient *neo4jHttpApiClient = nullptr;

private:
    inline static QNetworkAccessManager *networkAccessManager = nullptr;
};

//!
//! Test Neo4jHttpApiClient::queryDb() .
//!
TEST_F(Neo4jHttpApiClientTest, TestCase1) {
    const int id = 10;
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    std::optional<Neo4jHttpApiClient::QueryResponseSingleResult> response;
    neo4jHttpApiClient->queryDb(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MERGE (n:Test {id: $id})
                    SET n.text = $text
                    RETURN n.id AS id, n.text AS text;
                )!",
                QJsonObject {{"id", id}, {"text", now}}
            },
            // callback:
            [&response](const Neo4jHttpApiClient::QueryResponseSingleResult &response_) {
                response = response_;
            },
            app
    );

    bool ok = waitUntilHasValue(response);
    ASSERT_TRUE(ok) << "could not get query result before time-out";

    EXPECT_EQ(response.value().dbErrors.count(), 0);

    ASSERT_TRUE(response.value().getResult().has_value());
    const auto result = response.value().getResult().value();
    ASSERT_EQ(result.rowCount(), 1);

    const auto idValue = result.valueAt(0, "id");
    const auto textValue = result.valueAt(0, "text");
    EXPECT_EQ(idValue, QJsonValue(id));
    EXPECT_EQ(textValue, QJsonValue(now));
}

TEST_F(Neo4jHttpApiClientTest, OpenTransaction) {
    auto *transaction = neo4jHttpApiClient->getTransaction();

    openTransaction(transaction);
    EXPECT_TRUE(transaction->canQuery());

    delete transaction;
}

TEST_F(Neo4jHttpApiClientTest, CommitEmptyTransaction) {
    auto *transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    //
    std::optional<bool> commitOk;
    transaction->commit(
            [&commitOk](bool ok) { commitOk = ok; },
            app
    );
    bool ok = waitUntilHasValue(commitOk);
    ASSERT_TRUE(ok) << "no response from commit() within timeout";
    EXPECT_TRUE(commitOk.value());
    EXPECT_FALSE(transaction->canQuery());

    //
    delete transaction;
}

TEST_F(Neo4jHttpApiClientTest, RollbackEmptyTransaction) {
    auto *transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    //
    std::optional<bool> rollbackOk;
    transaction->rollback(
            [&rollbackOk](bool ok) { rollbackOk = ok; },
            app
    );
    bool ok = waitUntilHasValue(rollbackOk);
    ASSERT_TRUE(ok) << "no response from rollback() within timeout";
    EXPECT_TRUE(rollbackOk.value());
    EXPECT_FALSE(transaction->canQuery());

    //
    delete transaction;
}

TEST_F(Neo4jHttpApiClientTest, ErroneousQueryInTransaction) {
    using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult;

    auto *transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    // send a query that causes error
    std::optional<bool> queryOk;
    Neo4jHttpApiClient::QueryResponseSingleResult response;
    transaction->query(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MATCH (n:Test {id: $id})
                    RETURN n;
                )!",
                QJsonObject {} // parameter `id` missing
            },
            // callback:
            [&queryOk, &response](bool ok, const QueryResponseSingleResult &response_) {
                queryOk = ok;
                response = response_;
            },
            app
    );
    bool ok = waitUntilHasValue(queryOk);
    ASSERT_TRUE(ok) << "no response from query() before time-out";
    EXPECT_FALSE(queryOk.value());
    EXPECT_FALSE(response.dbErrors.isEmpty());
    EXPECT_FALSE(transaction->canQuery());

    //
    delete transaction;
}


TEST_F(Neo4jHttpApiClientTest, CommitTransaction) {
    using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult ;

    constexpr int id = 1001;

    // ==== transaction (1) ====
    auto *transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    //-- query
    const QString text = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    std::optional<bool> queryOk;
    transaction->query(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MERGE (n:Test {id: $id})
                    SET n.text = $text
                    RETURN n;
                )!",
                QJsonObject {{"id", id}, {"text", text}}
            },
            // callback:
            [&queryOk](bool ok, const Neo4jHttpApiClient::QueryResponseSingleResult &/*response*/) {
                queryOk = ok;
            },
            app
    );
    bool ok = waitUntilHasValue(queryOk);
    ASSERT_TRUE(ok) << "no response from query() within timeout";
    EXPECT_TRUE(queryOk.value());
    EXPECT_TRUE(transaction->canQuery());

    // -- commit transaction (1)
    std::optional<bool> commitOk;
    transaction->commit(
            [&commitOk](bool ok) { commitOk = ok; },
            app
    );
    ok = waitUntilHasValue(commitOk);
    ASSERT_TRUE(ok) << "no response from commit() within timeout";
    EXPECT_TRUE(commitOk.value());
    EXPECT_FALSE(transaction->canQuery());

    delete transaction;
    transaction = nullptr;

    // ==== transaction (2) ====
    transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    // -- query
    queryOk = std::nullopt;
    QString fetchedText;
    transaction->query(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MATCH (c:Test {id: $id})
                    RETURN c.text AS text
                )!",
                QJsonObject {{"id", id}}
            },
            // callback:
            [&queryOk, &fetchedText](bool ok, const QueryResponseSingleResult &response) {
                queryOk = ok;
                if (ok) {
                    if (response.getResult().has_value()) {
                        const auto result = response.getResult().value();
                        fetchedText = result.valueAt(0, "text").toString();
                    }
                }
            },
            app
    );

    ok = waitUntilHasValue(queryOk);
    ASSERT_TRUE(ok) << "no response from query() within timeout";
    EXPECT_TRUE(queryOk.value());

    EXPECT_EQ(fetchedText, text);

    delete transaction;
}

TEST_F(Neo4jHttpApiClientTest, RollbackTransaction) {
    using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult ;

    constexpr int id = 1001;

    // ==== transaction (1) ====
    auto *transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    // -- query 1
    std::optional<bool> queryOk;
    QString originalText;
    transaction->query(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MATCH (n:Test {id: $id})
                    RETURN n.text AS text;
                )!",
                QJsonObject {{"id", id}}
            },
            // callback:
            [&queryOk, &originalText](bool ok, const QueryResponseSingleResult &response) {
                queryOk = ok;
                if (ok) {
                    originalText = "";
                    if (response.getResult().has_value()) {
                        const auto textValue
                                = response.getResult().value().valueAt(0, "text"); // can be null
                        if (textValue.isString())
                            originalText = textValue.toString();
                    }
                }
            },
            app
    );
    bool ok = waitUntilHasValue(queryOk);
    ASSERT_TRUE(ok) << "no response from query() within timeout";
    EXPECT_TRUE(queryOk.value());

    // -- query 2: update the text
    queryOk = std::nullopt;
    const QString updatedText = originalText + "--updated";
    transaction->query(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MERGE (c:Test {id: $id})
                    SET c.text = $text;
                )!",
                QJsonObject {{"id", id}, {"text", updatedText}}
            },
            // callback:
            [&queryOk](bool ok, const QueryResponseSingleResult &/*response*/) {
                queryOk = ok;
            },
            app
    );
    ok = waitUntilHasValue(queryOk);
    ASSERT_TRUE(ok) << "no response from query() within timeout";
    EXPECT_TRUE(queryOk.value());

    // -- rollback transaction (1)
    std::optional<bool> rollbackOk;
    transaction->rollback(
            [&rollbackOk](bool ok) { rollbackOk = ok; },
            app
    );
    ok = waitUntilHasValue(rollbackOk);
    ASSERT_TRUE(ok) << "no response from rollback() within timeout";
    EXPECT_TRUE(rollbackOk.value());

    delete transaction;
    transaction = nullptr;

    // ==== transaction (2) ====
    transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    // -- query
    queryOk = std::nullopt;
    QString fetchedText;
    transaction->query(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MATCH (n:Test {id: $id})
                    RETURN n.text AS text;
                )!",
                QJsonObject {{"id", id}}
            },
            // callback:
            [&queryOk, &fetchedText](bool ok, const QueryResponseSingleResult &response) {
                queryOk = ok;
                if (ok) {
                    fetchedText = "";
                    if (response.getResult().has_value()) {
                        const auto textValue
                                = response.getResult().value().valueAt(0, "text"); // can be null
                        if (textValue.isString())
                            fetchedText = textValue.toString();
                    }
                }
            },
            app
    );
    ok = waitUntilHasValue(queryOk);
    ASSERT_TRUE(ok) << "no response from query() within timeout";
    EXPECT_TRUE(queryOk.value());

    delete transaction;

    // check that we get the original text
    EXPECT_EQ(fetchedText, originalText);
}

TEST_F(Neo4jHttpApiClientTest, TransactionKeptAlive) {
    auto *transaction = neo4jHttpApiClient->getTransaction();
    openTransaction(transaction);

    //
    qInfo().noquote() << "waiting for 3 sec...";
    QTest::qWait(3000);

    //
    std::optional<bool> rollbackOk;
    transaction->rollback(
            [&rollbackOk](bool ok) { rollbackOk = ok; },
            app
    );
    bool ok = waitUntilHasValue(rollbackOk);
    ASSERT_TRUE(ok) << "no response from rollback() within timeout";
    EXPECT_TRUE(rollbackOk.value());

    //
    delete transaction;
}
