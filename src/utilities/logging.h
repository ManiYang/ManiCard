#ifndef LOGGING_H
#define LOGGING_H

#include <optional>
#include <QFile>
#include <QMutex>
#include <QString>
#include <QTextStream>

// Note. Remember to add `DEFINES += QT_MESSAGELOGCONTEXT` in .pro file.

//!
//! \param applicationDirPath
//! \param logDirName: relative to `applicationDirPath`
//! \return the directory path, if the directory exists or is created successfully.
//!         Otherwise, return \c nullopt.
//!
std::optional<QString> createLogDir(const QString &applicationDirPath, const QString &logDirName);

//!
//! Delete log files that are
//! (1) not among the \a retainedFilesCriterion most recent files, and
//! (2) older than \a fileAgeDaysCriterion days.
//!
void deleteOldLogs(
        const QString &logDirPath, const int retainedFilesCriterion, const int fileAgeDaysCriterion);

struct LogFileStream
{
    QFile file;
    QTextStream textStream;
    QMutex mutex;
};

//!
//! \return successful?
//!
bool openLogFileStream(const QString &logDirPath, LogFileStream *logFileStream);

void closeLogFileStream(LogFileStream &logFileStream);

//!
//! This function is thread-safe.
//!
void writeMessageToLogFileStream(
        LogFileStream &logFileStream,
        QtMsgType msgType, const QMessageLogContext &context, const QString &msg);

//!
//! This function is thread-safe.
//!
void writeMessageToStdout(QtMsgType msgType, const QMessageLogContext &context, const QString &msg);

#endif // LOGGING_H
