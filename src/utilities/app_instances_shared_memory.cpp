#include <cstring>
#include <QDebug>
#include "app_instances_shared_memory.h"

AppInstancesSharedMemory::AppInstancesSharedMemory(
        const QString &semaphoreKey, const QString &sharedMemoryKey, QObject *parent)
    : QObject(parent)
    , semaphore(semaphoreKey, 1)
    , sharedMemory(new QSharedMemory(sharedMemoryKey, this))
{
    Q_ASSERT(semaphoreKey != sharedMemoryKey);

    // If the shared-memory segment exists but no process is currently attached to it, this
    // will destroy the shared-memory segment:
    semaphore.acquire();
    sharedMemory->attach();
    sharedMemory->detach();
    semaphore.release();
}

AppInstancesSharedMemory::~AppInstancesSharedMemory() {
    detach();
}

bool AppInstancesSharedMemory::tryCreateSharedMemory() {
    semaphore.acquire();
    bool ok = sharedMemory->create(sizeof(DataType));
    semaphore.release();

    if (!ok) {
        if (sharedMemory->error() != QSharedMemory::AlreadyExists) {
            qWarning().noquote()
                    << QString("error in creating share memory segment: %1")
                       .arg(sharedMemory->errorString());
        }
    }

    return ok;
}

bool AppInstancesSharedMemory::attach() {
    if (sharedMemory->isAttached())
        return true;

    semaphore.acquire();
    const bool ok = sharedMemory->attach();
    semaphore.release();

    if (!ok) {
        qWarning().noquote()
                << QString("error in attaching to the share memory segment: %1")
                   .arg(sharedMemory->errorString());
    }

    return ok;
}

void AppInstancesSharedMemory::detach() {
    if (!sharedMemory->isAttached())
        return;

    semaphore.acquire();
    bool ok = sharedMemory->detach();
    semaphore.release();

    if (!ok) {
        qWarning().noquote()
                << QString("error in detach from the share memory segment: %1")
                   .arg(sharedMemory->errorString());
    }
}

void AppInstancesSharedMemory::writeData(const DataType v) {
    if (!sharedMemory->isAttached()) {
        qWarning().noquote() << "Shared memory is not attached yet. Cannot write to it.";
        return;
    }

    semaphore.acquire();
    std::memcpy(sharedMemory->data(), &v, sizeof(DataType));
    semaphore.release();
}

AppInstancesSharedMemory::DataType AppInstancesSharedMemory::readAndClearData() {
    if (!sharedMemory->isAttached()) {
        qWarning().noquote() << "Shared memory is not attached yet. Cannot read from it.";
        return 0;
    }

    //
    semaphore.acquire();

    DataType v;
    std::memcpy(&v, sharedMemory->data(), sizeof(DataType));

    const DataType zero = 0;
    std::memcpy(sharedMemory->data(), &zero, sizeof(DataType));

    semaphore.release();

    return v;
}
