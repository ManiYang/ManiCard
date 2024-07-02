#include <iostream>
#include <QApplication>
#include <QDebug>
#include "global_constants.h"
#include "main_window.h"
#include "utilities/logging.h"

namespace {

LogFileStream logFileStream;

void messageHandlerWriteToLogFile(
        QtMsgType msgType, const QMessageLogContext &context, const QString &msg) {
    writeMessageToLogFileStream(logFileStream, msgType, context, msg);
}

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ManiCard");
    app.setQuitOnLastWindowClosed(true);

    // set up logging
    if constexpr (buildInReleaseMode) {
        const auto logDirOpt = createLogDir(app.applicationDirPath(), "logs");
        if (!logDirOpt.has_value())
            exit(EXIT_FAILURE);

        deleteOldLogs(
                logDirOpt.value(),
                40, // retainedFilesCriterion
                60 // fileAgeDaysCriterion
        );

        const bool ok = openLogFileStream(logDirOpt.value(), &logFileStream);
        if (!ok)
            exit(EXIT_FAILURE);

        qInstallMessageHandler(messageHandlerWriteToLogFile);
        qInfo() << "======== program start ========";
    }
    else {
        qInstallMessageHandler(writeMessageToStdout);
    }

    //
    MainWindow w;
    w.show();
    return app.exec();
}
