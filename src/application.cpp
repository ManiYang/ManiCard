#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include "application.h"
#include "app_data_readonly.h"
#include "services.h"
#include "utilities/async_routine.h"
#include "utilities/periodic_checker.h"
#include "widgets/app_style_sheet.h"
#include "widgets/main_window.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

Application::Application(QObject *parent)
        : QObject{parent} {
}

Application::~Application() {
    if (mainWindow != nullptr)
        delete mainWindow;
}

void Application::initialize() {
    mainWindow = new MainWindow;

    // set up connections
    connect(
            Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            qApp, [](const bool isDarkTheme) {
        qApp->setStyleSheet(isDarkTheme ? getDarkThemeStyleSheet() : getLightThemeStyleSheet());
    });

    connect(mainWindow, &MainWindow::userToReloadApp, mainWindow, [this]() {
        onUserToReloadApp();
    });
}

void Application::loadOnStart() {
    auto *routine = new AsyncRoutineWithErrorFlag;

    routine->addStep([this, routine]() {
        // let `mainWindow` prepare to reload and wait until mainWindow->canReload() returns true
        mainWindow->prepareToReload();

        (new PeriodicChecker)
                ->setPeriod(10)
                ->setTimeOut(6000)
                ->setPredicate([this]() { return mainWindow->canReload(); })
                ->onPredicateReturnsTrue([routine]() { routine->nextStep(); })
                ->onTimeOut([routine]() {
                    qWarning().noquote() << "time-out while awaiting MainWindow::canReload()";
                    routine->nextStep();
                })
                ->setAutoDelete()
                ->start();
    }, this);

    routine->addStep([this, routine]() {
        // show `mainWindow` and wait until it is visible
        mainWindow->show();

        (new PeriodicChecker)
                ->setPeriod(10)
                ->setTimeOut(6000)
                ->setPredicate([this]() { return mainWindow->isVisible(); })
                ->onPredicateReturnsTrue([routine]() { routine->nextStep(); })
                ->onTimeOut([routine]() {
                    qWarning().noquote() << "time-out while awaiting MainWindow::isVisible()";
                    routine->nextStep();
                })
                ->setAutoDelete()
                ->start();
    }, this);

    routine->addStep([this, routine]() {
        // reload
        ContinuationContext context(routine);
        reload([this](bool ok) {
            if (!ok)
                QMessageBox::warning(mainWindow, " ", "Failed to load data.");
        });
    }, this);

    routine->start();
}

void Application::activateMainWindow() {
    mainWindow->activateWindow();
}

void Application::reload(std::function<void (bool)> callback) {
    // set application style sheet
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    qApp->setStyleSheet(isDarkTheme ? getDarkThemeStyleSheet() : getLightThemeStyleSheet());

    // let `mainWindow` reload data
    mainWindow->load([callback](bool ok) {
        if (!ok)
            qWarning().noquote() << "MainWindow failed to load data";
        callback(ok);
    });
}

void Application::onUserToReloadApp() {
    auto *routine = new AsyncRoutineWithErrorFlag;

    routine->addStep([this, routine]() {
        ContinuationContext context(routine);
        mainWindow->setEnabled(false);
    }, this);

    routine->addStep([this, routine]() {
        // wait for MainWindow::canReload() and wait until mainWindow->canReload() returns true
        mainWindow->prepareToReload();

        (new PeriodicChecker)
                ->setPeriod(10)
                ->setTimeOut(6000)
                ->setPredicate([this]() { return mainWindow->canReload(); })
                ->onPredicateReturnsTrue([routine]() { routine->nextStep(); })
                ->onTimeOut([routine]() {
                    qWarning().noquote() << "time-out while awaiting MainWindow::canReload()";
                    routine->nextStep();
                })
                ->setAutoDelete()
                ->start();
    }, this);

    routine->addStep([this, routine]() {
        // clear cache and reload
        ContinuationContext context(routine);

        Services::instance()->clearPersistedDataAccessCache();
        reload([this](bool ok) {
            if (!ok)
                QMessageBox::warning(mainWindow, " ", "Reload failed.");
        });

        mainWindow->setEnabled(true);
    }, this);

    routine->start();
}
