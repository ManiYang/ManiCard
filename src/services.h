#ifndef SERVICES_H
#define SERVICES_H

#include <QNetworkAccessManager>
#include <QString>

class AppData;
class BoardsDataAccess;
class CardsDataAccess;
class LocalSettingsFile;
class Neo4jHttpApiClient;
class PersistedDataAccess;
class QueuedDbAccess;
class UnsavedUpdateRecordsFile;

class Services
{
private:
    Services();
public:
    ~Services();
    static Services *instance();

    Services(const Services &) = delete;
    Services &operator =(const Services &) = delete;

    //!
    //! \return true iff successful
    //!
    bool initialize(QString *errorMsg = nullptr);

    AppData *getAppData() const;
    PersistedDataAccess *getPersistedDataAccess() const;

    QString errorMsgOnUnsavedUpdate(const QString &what) const;

private:
    QNetworkAccessManager *networkAccessManager {nullptr};
    Neo4jHttpApiClient *neo4jHttpApiClient {nullptr};
    std::shared_ptr<CardsDataAccess> cardsDataAccess;
    std::shared_ptr<BoardsDataAccess> boardsDataAccess;
    QueuedDbAccess *queuedDbAccess {nullptr};
    std::shared_ptr<LocalSettingsFile> localSettingsFile;
    std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile;
    PersistedDataAccess *persistedDataAccess {nullptr};
    AppData *appData {nullptr};

    QString unsavedUpdateFilePath;
};

#endif // SERVICES_H
