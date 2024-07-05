#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include "neo4j_http_api_client.h"
#include "services.h"
#include "utilities/json_util.h"

constexpr char configFile[] = "config.json";

Services::Services() {}

Services::~Services() {}

Services *Services::instance() {
    static Services obj;
    return &obj;
}

bool Services::initialize(QString *errorMsg) {
    try {
        // read config file (see src/config.example.json)
        QString err;
        auto doc = readJsonFile(
                QDir(qApp->applicationDirPath()).filePath(configFile),
                &err);
        if (doc.isNull())
            throw std::runtime_error(err.toStdString());
        const QJsonObject config = doc.object();

        // create services
        networkAccessManager = new QNetworkAccessManager(qApp);

        try {
            neo4jHttpApiClient = new Neo4jHttpApiClient(
                    JsonReader(config)["neo4j_db"]["http_url"].getStringOrThrow(),
                    JsonReader(config)["neo4j_db"]["database"].getStringOrThrow(),
                    JsonReader(config)["neo4j_db"]["auth_file"].getStringOrThrow(),
                    networkAccessManager,
                    qApp);
        }
        catch (JsonReaderError &e) {
            throw std::runtime_error(
                    QString("error in reading config: %1").arg(e.what()).toStdString());
        }



    }
    catch (std::runtime_error &e) {
        if (errorMsg)
            *errorMsg = e.what();
        return false;
    }

    return true;
}
