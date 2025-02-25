#include <iostream>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include "application.h"
#include "global_constants.h"
#include "services.h"
#include "utilities/app_instances_shared_memory.h"
#include "utilities/fonts_util.h"
#include "utilities/logging.h"
#include "widgets/main_window.h"

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
    }
    else {
        qInstallMessageHandler(writeMessageToStdout);
    }

    //
    qInfo() << "======== program start ========";

    // font
    {
        QFont font = app.font("QMenu");
        font.setPointSize(10);
        app.setFont(font);
    }
    {
        const auto allFamilies = getFontFamilies();
        const QStringList monospaceCandidates {
            "JetBrains Mono", "Menlo", "Source Code Pro", "Lucida Console", "Courier New"};

        QString monospaceFamily;
        for (const QString &family: monospaceCandidates) {
            if (allFamilies.contains(family)) {
                monospaceFamily = family;
                break;
            }
        }

        app.setProperty("monospaceFontFamily", monospaceFamily);
    }

    //
    auto *sharedMemActivateFlag = new AppInstancesSharedMemory(
            "semaphore.activateFlag.ManiCard.Ca4NHCmSoHUc4urL", // semaphoreKey
            "sharedMemory.activateFlag.ManiCard.Ca4NHCmSoHUc4urL" // sharedMemoryKey
    ); // a shared memory segment used as an inter-process "activate-main-window" flag
    auto *timerReadSharedMemActivateFlag = new QTimer;

    QTimer::singleShot(
                0, &app,
                [&app, sharedMemActivateFlag, timerReadSharedMemActivateFlag]() {
        const bool sharedMemoryCreated = sharedMemActivateFlag->tryCreateSharedMemory();
        if (!sharedMemoryCreated) {
            std::cout << "another process of this app is already running" << std::endl;
            sharedMemActivateFlag->attach();
            sharedMemActivateFlag->writeData(1); // set "activate-main-window" flag
            app.quit();
            return;
        }

        //
        auto *application = new Application(&app);

        // start polling against "activate-main-window" flag
        QObject::connect(
                timerReadSharedMemActivateFlag, &QTimer::timeout,
                &app, [application, sharedMemActivateFlag]() {
            const qint16 v = sharedMemActivateFlag->readAndClearData();
            if (v != 0)
                application->activateMainWindow();
        });
        timerReadSharedMemActivateFlag->start(1000);

        // initialize services
        QString errorMsg;
        bool ok = Services::instance()->initialize(&errorMsg);
        if (!ok)
        {
            QMessageBox::critical(nullptr, "Error", errorMsg);
            app.quit();
            return;
        }

        //
        application->initialize();
        application->loadOnStart();
    });

    //
    const int returnCode = app.exec();
    qInfo().noquote() << QString("app exited with code %1").arg(returnCode);

    // clean up
    delete sharedMemActivateFlag;
    delete timerReadSharedMemActivateFlag;

    if constexpr (buildInReleaseMode)
        closeLogFileStream(logFileStream);

    return returnCode;
}
