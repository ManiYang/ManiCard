#ifndef UNSAVEDUPDATERECORDSFILE_H
#define UNSAVEDUPDATERECORDSFILE_H

#include <QString>

class UnsavedUpdateRecordsFile
{
public:
    explicit UnsavedUpdateRecordsFile(const QString &filePath);

    void append(const QString &time, const QString &title, const QString &details);

    QString getFilePath() const;

private:
    const QString filePath;
};

#endif // UNSAVEDUPDATERECORDSFILE_H
