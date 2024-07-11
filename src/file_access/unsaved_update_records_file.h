#ifndef UNSAVEDUPDATERECORDSFILE_H
#define UNSAVEDUPDATERECORDSFILE_H

#include <QString>

class UnsavedUpdateRecordsFile
{
public:
    UnsavedUpdateRecordsFile();

    void append(const QString &time, const QString &title, const QString &details);

private:
    const QString filePath;
};

#endif // UNSAVEDUPDATERECORDSFILE_H
