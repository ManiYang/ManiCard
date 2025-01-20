#include <QApplication>
#include "app_data_readonly.h"
#include "app_setup.h"
#include "services.h"
#include "widgets/app_style_sheet.h"

void setUpApp() {
    // dark theme and application style sheet
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    qApp->setStyleSheet(isDarkTheme ? getDarkThemeStyleSheet() : getLightThemeStyleSheet());

    QObject::connect(
            Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            qApp, [](const bool isDarkTheme) {
        qApp->setStyleSheet(isDarkTheme ? getDarkThemeStyleSheet() : getLightThemeStyleSheet());
    });
}
