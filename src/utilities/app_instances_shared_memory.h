#ifndef APP_INSTANCES_SHARED_MEMORY_H
#define APP_INSTANCES_SHARED_MEMORY_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

class AppInstancesSharedMemory : public QObject
{
    Q_OBJECT
public:
    //!
    //! \param semaphoreKey
    //! \param sharedMemoryKey: must be different from \e semaphoreKey
    //! \param parent
    //!
    explicit AppInstancesSharedMemory(
            const QString &semaphoreKey, const QString &sharedMemoryKey, QObject *parent = nullptr);
    ~AppInstancesSharedMemory();

    //!
    //! If the shared memory segment already exists, the creation will fail.
    //! If the creation is successful, also attach to the created shared memory segment.
    //! \return successful?
    //!
    bool tryCreateSharedMemory();

    bool attach();
    void detach();

    //
    using DataType = qint16;
    void writeData(const DataType v);
    DataType readAndClearData();

private:
    QSystemSemaphore semaphore; // serves as a cross-process mutex to the shared memory
    QSharedMemory *sharedMemory;
};

#endif // APP_INSTANCES_SHARED_MEMORY_H
