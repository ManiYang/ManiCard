#ifndef LOCAL_SETTINGS_FILE_H
#define LOCAL_SETTINGS_FILE_H

#include <optional>
#include <QJsonObject>
#include <QPointF>
#include <QString>

class LocalSettingsFile
{
public:
    explicit LocalSettingsFile(const QString &appLocalDataDir);

    //!
    //! It's not an error if the value is not found.
    //! \return (ok?, result)
    //!
    std::pair<bool, std::optional<int>> readLastOpenedBoardId();

    //!
    //! It's not an error if the value is not found.
    //! \return (ok?, result)
    //!
    std::pair<bool, std::optional<QPointF>> readTopLeftPosOfBoard(const int boardId);

    bool writeLastOpenedBoardId(const int lastOpenedBoardId);
    bool writeTopLeftPosOfBoard(const int boardId, const QPointF &topLeftPos);
    bool removeBoard(const int boardId);

private:
    QString filePath;

    QJsonObject read();
    bool write(const QJsonObject &obj);

    static QJsonObject defaultSettings();
};

#endif // LOCAL_SETTINGS_FILE_H
