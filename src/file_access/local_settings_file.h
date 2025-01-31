#ifndef LOCAL_SETTINGS_FILE_H
#define LOCAL_SETTINGS_FILE_H

#include <optional>
#include <QJsonObject>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>

class LocalSettingsFile
{
public:
    explicit LocalSettingsFile(const QString &appLocalDataDir);

    // ==== read operations ====

    // It's not an error if the value is not found. Return (ok?, result).

    std::pair<bool, std::optional<bool>> readIsDarkTheme();
    std::pair<bool, std::optional<bool>> readAutoAdjustCardColorForDarkTheme();
    std::pair<bool, std::optional<int>> readLastOpenedBoardIdOfWorkspace(const int workspaceId);
    std::pair<bool, std::optional<int>> readLastOpenedWorkspaceId();
    std::pair<bool, std::optional<QPointF>> readTopLeftPosOfBoard(const int boardId);
    std::pair<bool, std::optional<QRect>> readMainWindowSizePos();
    std::pair<bool, std::optional<QString>> readExportOutputDirectory();

    // ==== write operations ====

    bool writeIsDarkTheme(const bool isDarkTheme);
    bool writeAutoAdjustCardColorForDarkTheme(const bool autoAdjust);
    bool writeLastOpenedBoardIdOfWorkspace(const int workspaceId, const int lastOpenedBoardId);
    bool writeLastOpenedWorkspaceId(const int lastOpenedWorkspaceId);
    bool writeTopLeftPosOfBoard(const int boardId, const QPointF &topLeftPos);
    bool removeBoard(const int boardId);
    bool writeMainWindowSizePos(const QRect &rect);
    bool writeExportOutputDirectory(const QString &outputDir);

private:
    QString filePath;

    QJsonObject read();
    bool write(const QJsonObject &obj);

    static QJsonObject defaultSettings();
};

#endif // LOCAL_SETTINGS_FILE_H
