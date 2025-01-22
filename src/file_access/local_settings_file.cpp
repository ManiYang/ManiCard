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
 *   "appearance": {
 *     "isDarkTheme": false,
 *     "autoAdjustCardColorsForDarkTheme: false
 *   }
 *   "mainWindow": {
 *     "size": [1000, 800],
 *     "pos: [200, 100]
 *   },
 *   "workspaces": {
 *     "lastOpenedWorkspaceId": 10,
 *     "0": {
 *       "lastOpenedBoardId": 12
 *     },
 *     "1": {
 *       "lastOpenedBoardId": 34
 *     }
 *   },
 *   "boards": {
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

constexpr char sectionAppearance[] = "appearance";
constexpr char sectionMainWindow[] = "mainWindow";
constexpr char sectionWorkspaces[] = "workspaces";
constexpr char sectionBoards[] = "boards";

constexpr char keySize[] = "size";
constexpr char keyPos[] = "pos";
constexpr char keyLastOpenedBoardId[] = "lastOpenedBoardId";
constexpr char keyLastOpenedWorkspaceId[] = "lastOpenedWorkspaceId";
constexpr char keyTopLeftPos[] = "topLeftPos";
constexpr char keyIsDarkTheme[] = "isDarkTheme";
constexpr char keyAutoAdjustCardColorsForDarkTheme[] = "autoAdjustCardColorsForDarkTheme";

LocalSettingsFile::LocalSettingsFile(const QString &appLocalDataDir)
        : filePath(QDir(appLocalDataDir).filePath(fileName)) {
    Q_ASSERT(!appLocalDataDir.isEmpty());
}

std::pair<bool, std::optional<bool> > LocalSettingsFile::readIsDarkTheme() {
    const QJsonObject obj = read();
    const QJsonValue v = JsonReader(obj)[sectionAppearance][keyIsDarkTheme].get();
    if (v.isUndefined())
        return {true, std::nullopt};
    else
        return {true, v.toBool()};
}

std::pair<bool, std::optional<bool> > LocalSettingsFile::readAutoAdjustCardColorForDarkTheme() {
    const QJsonObject obj = read();
    const QJsonValue v
            = JsonReader(obj)[sectionAppearance][keyAutoAdjustCardColorsForDarkTheme].get();
    if (v.isUndefined())
        return {true, std::nullopt};
    else
        return {true, v.toBool()};
}

std::pair<bool, std::optional<int> > LocalSettingsFile::readLastOpenedBoardIdOfWorkspace(
        const int workspaceId) {
    const QJsonObject obj = read();
    const QJsonValue v
            = JsonReader(obj)[sectionWorkspaces][QString::number(workspaceId)][keyLastOpenedBoardId].get();
    if (v.isUndefined())
        return {true, std::optional<int>()};

    if (!jsonValueIsInt(v)) {
        qWarning().noquote()
                << QString("value of %1 for workspace %2 is not an integer")
                   .arg(keyLastOpenedBoardId).arg(workspaceId);
        return {false, std::optional<int>()};
    }

    return {true, v.toInt()};
}

std::pair<bool, std::optional<int> > LocalSettingsFile::readLastOpenedWorkspaceId() {
    const QJsonObject obj = read();
    const QJsonValue v = JsonReader(obj)[sectionWorkspaces][keyLastOpenedWorkspaceId].get();
    if (v.isUndefined())
        return {true, std::optional<int>()};

    if (!jsonValueIsInt(v)) {
        qWarning().noquote()
                << QString("value of %1 is not an integer").arg(keyLastOpenedWorkspaceId);
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

std::pair<bool, std::optional<QRect> > LocalSettingsFile::readMainWindowSizePos() {
    const QJsonObject obj = read();
    const QJsonValue sizeValue = JsonReader(obj)[sectionMainWindow][keySize].get();
    const QJsonValue posValue = JsonReader(obj)[sectionMainWindow][keyPos].get();

    std::optional<QSize> sizeOpt;
    if (jsonValueIsArrayOfSize(sizeValue, 2))
        sizeOpt = QSize(sizeValue[0].toInt(), sizeValue[1].toInt());

    std::optional<QPoint> posOpt;
    if (jsonValueIsArrayOfSize(posValue, 2))
        posOpt = QPoint(posValue[0].toInt(), posValue[1].toInt());

    if (sizeOpt.has_value() && posOpt.has_value())
        return {true, QRect(posOpt.value(), sizeOpt.value())};
    else
        return {true, std::nullopt};
}

bool LocalSettingsFile::writeIsDarkTheme(const bool isDarkTheme) {
    QJsonObject obj = read();

    // set obj[sectionAppearance][keyIsDark] = isDarkTheme
    QJsonObject appearanceObj = obj[sectionAppearance].toObject();
    appearanceObj[keyIsDarkTheme] = isDarkTheme;

    obj[sectionAppearance] = appearanceObj;

    //
    const bool ok = write(obj);
    return ok;
}

bool LocalSettingsFile::writeAutoAdjustCardColorForDarkTheme(const bool autoAdjust) {
    QJsonObject obj = read();

    // set obj[sectionAppearance][keyAutoAdjustCardColorsForDarkTheme] = autoAdjust
    QJsonObject appearanceObj = obj[sectionAppearance].toObject();
    appearanceObj[keyAutoAdjustCardColorsForDarkTheme] = autoAdjust;

    obj[sectionAppearance] = appearanceObj;

    //
    const bool ok = write(obj);
    return ok;
}

bool LocalSettingsFile::writeLastOpenedBoardIdOfWorkspace(
        const int workspaceId, const int lastOpenedBoardId) {
    QJsonObject obj = read();

    // set obj[sectionWorkspaces][Str(workspaceId)][keyLastOpenedBoardId] = lastOpenedBoardId
    QJsonObject workspacesObj = obj[sectionWorkspaces].toObject();
    QJsonObject workspaceObj = workspacesObj[QString::number(workspaceId)].toObject();
    workspaceObj[keyLastOpenedBoardId] = lastOpenedBoardId;

    workspacesObj[QString::number(workspaceId)] = workspaceObj;
    obj[sectionWorkspaces] = workspacesObj;

    //
    const bool ok = write(obj);
    return ok;
}

bool LocalSettingsFile::writeLastOpenedWorkspaceId(const int lastOpenedWorkspaceId) {
    QJsonObject obj = read();

    // set obj[sectionWorkspaces][keyLastOpenedWorkspaceId] = lastOpenedWorkspaceId
    QJsonObject workspacesObj = obj[sectionWorkspaces].toObject();
    workspacesObj[keyLastOpenedWorkspaceId] = lastOpenedWorkspaceId;

    obj[sectionWorkspaces] = workspacesObj;

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

bool LocalSettingsFile::writeMainWindowSizePos(const QRect &rect) {
    QJsonObject obj = read();

    // set obj[sectionMainWindow][keySize] = rect.size() and
    // obj[sectionMainWindow][keyPos] = rect.topLeft()
    QJsonObject mainWindowObj = obj[sectionMainWindow].toObject();
    mainWindowObj[keySize] = QJsonArray {rect.width(), rect.height()};
    mainWindowObj[keyPos] = QJsonArray {rect.x(), rect.y()};

    obj[sectionMainWindow] = mainWindowObj;

    //
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
