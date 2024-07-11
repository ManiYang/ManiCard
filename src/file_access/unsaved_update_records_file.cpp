#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include "unsaved_update_records_file.h"
#include "utilities/json_util.h"

constexpr char filePath_[] = "./unsaved_updates.txt";

UnsavedUpdateRecordsFile::UnsavedUpdateRecordsFile()
    : filePath(filePath_) {
}

void UnsavedUpdateRecordsFile::append(
        const QString &time, const QString &title, const QString &details) {
    QFile file(filePath);
    const bool ok = file.open(QIODevice::Append);
    if (!ok) {
        qWarning().noquote() << QString("Could not open %1 for appending").arg(filePath);
        return;
    }

    file.write(QString("[%1] %2\n").arg(time, title).toUtf8());
    const auto detailsLines = details.split("\n");
    for (const QString &line: detailsLines) {
        file.write(QString("  %1\n").arg(line).toUtf8());
    }
    file.write("\n");
}
