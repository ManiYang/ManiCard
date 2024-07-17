#ifndef APP_LOCAL_DATA_DIR_H
#define APP_LOCAL_DATA_DIR_H

#include <QDir>
#include <QStandardPaths>
#include <QString>

//!
//! The returned directory is created.
//! \return "" if failed
//!
inline QString getAppLocalDataDir(QString *errorMsg = nullptr) {
    const QString appDataDirPath
            = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (appDataDirPath.isEmpty()) {
        if (errorMsg)
            *errorMsg = "Could not get a writable app data directory.";
        return "";
    }

    QDir appDataDir(appDataDirPath);
    if (!appDataDir.exists()) {
        const bool ok = appDataDir.mkpath(".");
        if (!ok) {
            if (errorMsg)
                *errorMsg = QString("Could not create directory %1").arg(appDataDirPath);
            return "";
        }
    }

    return appDataDir.absolutePath();
}

#endif // APP_LOCAL_DATA_DIR_H
