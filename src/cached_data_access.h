#ifndef CACHEDDATAACCESS_H
#define CACHEDDATAACCESS_H

#include <QObject>
#include <QPointer>
#include "models/card.h"

class QueuedDbAccess;
class UnsavedUpdateRecordsFile;

class CachedDataAccess : public QObject
{
    Q_OBJECT
public:
    explicit CachedDataAccess(
            QueuedDbAccess *queuedDbAccess_,
            std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_,
            QObject *parent = nullptr);

    // ==== read ====

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext);

    // ==== write ====

    // If a write operation failed, a record of unsaved update is added.

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);


private:
    QueuedDbAccess *queuedDbAccess;
    std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile;

    // data cache
    struct Cache
    {
        QHash<int, Card> cards;
    };
    Cache cache;


};

/*
 * read:
 *   1. get the parts that are already cached
 *   2. query DB for the other parts (if any)
 *      + if successful: update cache
 *      + if failed: whole process fails
 * write:
 *   1. update cache
 *   2. write DB
 *   3. if step 2 failed, add to unsaved updates
*/

#endif // CACHEDDATAACCESS_H
