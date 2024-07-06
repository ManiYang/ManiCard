#ifndef CACHEDDATAACCESS_H
#define CACHEDDATAACCESS_H

#include <QObject>

class CachedDataAccess : public QObject
{
    Q_OBJECT
public:
    explicit CachedDataAccess(QObject *parent = nullptr);

private:


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
