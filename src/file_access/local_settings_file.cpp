#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>
#include "local_settings_file.h"
#include "utilities/json_util.h"

/* example:
 *
 * {
 *   "boards": {
 *     "lastOpenedBoardId": 0,
 *     "0": {
 *       "topLeftPos": [100, -50]
 *     },
 *     "1": {
 *       "topLeftPos": [300, -20]
 *     }
 *   }
 * }
*/

constexpr char fileName[] = "user_settings.json";
constexpr char sectionBoards[] = "boards";
constexpr char keyLastOpenedBoardId[] = "lastOpenedBoardId";
constexpr char keyTopLeftPos[] = "topLeftPos";

LocalSettingsFile::LocalSettingsFile(const QString &appLocalDataDir)
        : filePath(QDir(appLocalDataDir).filePath(fileName)) {
    Q_ASSERT(!appLocalDataDir.isEmpty());
}

std::pair<bool, std::optional<int> > LocalSettingsFile::readLastOpenedBoardId() {
    const QJsonObject obj = read();
    const QJsonValue v = JsonReader(obj)[sectionBoards][keyLastOpenedBoardId].get();
    if (v.isUndefined())
        return {true, std::optional<int>()};

    if (!jsonValueIsInt(v)) {
        qWarning().noquote()
                << QString("value of %1 is not an integer").arg(keyLastOpenedBoardId);
        return {false, std::optional<int>()};
    }

    return {true, v.toInt()};
}

std::pair<bool, std::optional<QPointF> >
LocalSettingsFile::readTopLeftPosOfBoard(const int boardId) {
    const QJsonObject obj = read();
    const QJsonValue v
            = JsonReader(obj)[sectionBoards][QString::number(boardId)][keyTopLeftPos].get();
    if (v.isUndefined())
        return {true, std::nullopt};

    if (!jsonValueIsArrayOfSize(v, 2)) {
        qWarning().noquote()
                << QString("value of %1 for board %2 is not an array of size 2")
                   .arg(keyTopLeftPos).arg(boardId);
        return {false, std::nullopt};
    }

    return {true, QPointF(v[0].toDouble(), v[1].toDouble())};
}

bool LocalSettingsFile::writeLastOpenedBoardId(const int lastOpenedBoardId) {
    QJsonObject obj = read();

    // set obj[sectionBoards][keyLastOpenedBoardId] = lastOpenedBoardId
    QJsonObject boardsObj = obj[sectionBoards].toObject();
    boardsObj[keyLastOpenedBoardId] = lastOpenedBoardId;

    obj[sectionBoards] = boardsObj;

    //
    const bool ok = write(obj);
    return ok;
}

bool LocalSettingsFile::writeTopLeftPosOfBoard(const int boardId, const QPointF &topLeftPos) {
    QJsonObject obj = read();

    // set obj[sectionBoards][Str(boardId)][keyTopLeftPos] = topLeftPos
    QJsonObject boardsObj = obj[sectionBoards].toObject();
    QJsonObject boardObj = boardsObj[QString::number(boardId)].toObject();
    boardObj[keyTopLeftPos] = QJsonArray {topLeftPos.x(), topLeftPos.y()};

    boardsObj[QString::number(boardId)] = boardObj;
    obj[sectionBoards] = boardsObj;

    //
    const bool ok = write(obj);
    return ok;
}

bool LocalSettingsFile::removeBoard(const int boardId) {
    QJsonObject obj = read();

    QJsonObject boardsObj = obj[sectionBoards].toObject();
    boardsObj.remove(QString::number(boardId));

    obj[sectionBoards] = boardsObj;

    const bool ok = write(obj);
    return ok;
}

QJsonObject LocalSettingsFile::read() {
    if (!QFileInfo::exists(filePath))
        return QJsonObject {};

    QJsonDocument doc = readJsonFile(filePath);
    if (!doc.isObject()) {
        qWarning().noquote()
                << QString("File %1 is corrupted. Will be replaced by default one.").arg(filePath);
        return defaultSettings();
    }

    return doc.object();
}

bool LocalSettingsFile::write(const QJsonObject &obj) {
    QSaveFile file(filePath);
    const bool openOk = file.open(QIODevice::WriteOnly);
    if (!openOk) {
        qWarning().noquote() << QString("could not open %1 for writing").arg(filePath);
        return false;
    }

    file.write(QJsonDocument(obj).toJson());

    const bool writeOk = file.commit();
    if (!writeOk) {
        qWarning().noquote()
                << QString("Failed to write to %1: %2").arg(filePath, file.errorString());
    }
    return writeOk;
}

QJsonObject LocalSettingsFile::defaultSettings() {
    return {
        {sectionBoards, QJsonObject {}}
    };
}
