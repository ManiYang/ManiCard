#ifndef LOCAL_SETTINGS_FILE_H
#define LOCAL_SETTINGS_FILE_H

#include <optional>
#include <QJsonObject>
#include <QPointF>
#include <QSize>
#include <QString>

class LocalSettingsFile
{
public:
    explicit LocalSettingsFile(const QString &appLocalDataDir);

    // ==== read operations ====

    // It's not an error if the value is not found. Return (ok?, result).

    std::pair<bool, std::optional<int>> readLastOpenedBoardId();
    std::pair<bool, std::optional<QPointF>> readTopLeftPosOfBoard(const int boardId);
    std::pair<bool, std::optional<QSize>> readMainWindowSize();

    // ==== write operations ====

    bool writeLastOpenedBoardId(const int lastOpenedBoardId);
    bool writeTopLeftPosOfBoard(const int boardId, const QPointF &topLeftPos);
    bool removeBoard(const int boardId);
    bool writeMainWindowSize(const QSize &size);

private:
    QString filePath;

    QJsonObject read();
    bool write(const QJsonObject &obj);

    static QJsonObject defaultSettings();
};

#endif // LOCAL_SETTINGS_FILE_H
