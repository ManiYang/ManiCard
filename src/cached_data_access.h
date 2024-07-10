#ifndef CACHEDDATAACCESS_H
#define CACHEDDATAACCESS_H

#include <QObject>
#include <QPointer>
#include "models/card.h"

class QueuedDbAccess;

class CachedDataAccess : public QObject
{
    Q_OBJECT
public:
    explicit CachedDataAccess(QueuedDbAccess *queuedDbAccess_, QObject *parent = nullptr);

    // ==== read ====

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext);

    // ==== write ====


private:
    QueuedDbAccess *queuedDbAccess;

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
