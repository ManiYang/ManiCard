#include <iostream>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QMutex>
#include <QTextStream>
#include "logging.h"

std::optional<QString> createLogDir(const QString &applicationDirPath, const QString &logDirName) {
    const QString logDirPath = QDir(applicationDirPath).filePath(logDirName);

    if (!QDir(logDirPath).exists()) {
        QDir appDir(QCoreApplication::applicationDirPath());
        const bool createOk = appDir.mkdir(logDirName);
        if (!createOk) {
            std::cout << "could not create directory" << logDirPath.toStdString() << std::endl;
            return std::nullopt;
        }
    }
    return logDirPath;
}

void deleteOldLogs(
        const QString &logDirPath, const int retainedFilesCriterion,
        const int fileAgeDaysCriterion) {
    QDir logDir(logDirPath);
    const QStringList logFiles
            = logDir.entryList(
                  {"log_*.txt"},
                  QDir::Files,
                  QDir::Name | QDir::Reversed // sort
              )
              .mid(retainedFilesCriterion);

    const QString dateCriterion
            = QDate::currentDate().addDays(-fileAgeDaysCriterion).toString("yyyyMMdd");
    const QString logFileNameCriterion = QString("log_%1.txt").arg(dateCriterion);
    for (const QString &file: logFiles) {
        if (file < logFileNameCriterion)
            logDir.remove(file);
    }
}

bool openLogFileStream(const QString &logDirPath, LogFileStream *logFileStream) {
    const QString today = QDate::currentDate().toString("yyyyMMdd");
    const QString logFileName = QString("log_%1.txt").arg(today);

    logFileStream->file.setFileName(QDir(logDirPath).filePath(logFileName));
    const bool ok = logFileStream->file.open(
            QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    if (!ok) {
        std::cout << "could not open file for appending:"
                  << logFileStream->file.fileName().toStdString() << std::endl;
        return false;
    }

    logFileStream->textStream.setDevice(&logFileStream->file);
    return true;
}

void closeLogFileStream(LogFileStream &logFileStream) {
    if (logFileStream.file.isOpen()) {
        QMutexLocker locker(&logFileStream.mutex);

        logFileStream.textStream.flush();
        logFileStream.file.close();
    }
}

QString getMsgTypeName(const QtMsgType msgType) {
    const static QHash<int, QString> msgTypeToLabel {
        {QtDebugMsg, "debug"}, {QtInfoMsg, "info"}, {QtWarningMsg, "warning"},
        {QtCriticalMsg, "error"}, {QtFatalMsg, "error"}, {QtSystemMsg, "system"}
    };
    return msgTypeToLabel.value(msgType);
}

void writeMessageToLogFileStream(
        LogFileStream &logFileStream,
        QtMsgType msgType, const QMessageLogContext &context, const QString &msg) {
     if (!logFileStream.file.isOpen())
         return;

    const QString contextFile = context.file ? QFileInfo(QString(context.file)).baseName() : "";
    const QString formattedMsg
            = QString("%1 [%2] (%3) %4\n").arg(
                    QDateTime::currentDateTime().toString("MM/dd HH:mm:ss"),
                    getMsgTypeName(msgType),
                    contextFile,
                    msg);

    {
        QMutexLocker locker(&logFileStream.mutex);
        logFileStream.textStream << formattedMsg;
        logFileStream.textStream.flush();
    }
}

void writeMessageToStdout(
        QtMsgType msgType, const QMessageLogContext &context, const QString &msg) {
    QString contextFile = context.file ? QFileInfo(QString(context.file)).baseName() : "";
    const QString outputText
            = QString("%1 [%2] (%3) %4").arg(
                    QDateTime::currentDateTime().toString("HH:mm:ss.zzz"),
                    getMsgTypeName(msgType),
                    contextFile,
                    msg);
    std::cout << outputText.toLocal8Bit().constData() << std::endl;
}
