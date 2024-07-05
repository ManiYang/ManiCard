#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include "neo4j_http_api_client.h"
#include "services.h"
#include "utilities/json_util.h"

constexpr char configFilePath[] = "./config.json";

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
        auto doc = readJsonFile(configFilePath, &err);
        if (doc.isNull())
            throw std::runtime_error(err.toStdString());
        const QJsonObject config = doc.object();

        // create services
        networkAccessManager = new QNetworkAccessManager(qApp);

        try {
            neo4jHttpApiClient = new Neo4jHttpApiClient(
                    JsonReader(config)["neo4j_db"]["http_url"].getString(),
                    JsonReader(config)["neo4j_db"]["database"].getString(),
                    JsonReader(config)["neo4j_db"]["auth_file"].getString(),
                    networkAccessManager,
                    qApp);
        }
        catch (JsonReaderException &e) {
//            ....



        }



    }
    catch (std::runtime_error &e) {
        if (errorMsg)
            *errorMsg = QString::fromStdString(e.what());
        return false;
    }

    return true;
}
