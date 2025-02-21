#ifndef FILENAMES_UTIL_H
#define FILENAMES_UTIL_H

#include <QRegularExpression>
#include <QString>

inline QString makeValidFileName(const QString &s) {
    const static QRegularExpression regexpSlashesAndPipe(R"%([\\/\|])%");

    QString result = s;

    // replaces characters that cannot be in a file name
    result.replace("*", "＊");
    result.replace("\"", "'");
    result.replace(regexpSlashesAndPipe, "_");
    result.replace("<", "_");
    result.replace(">", "_");
    result.replace(":", "：");
    result.replace("?", "？");

    //
    return result;
}

#endif // FILENAMES_UTIL_H
