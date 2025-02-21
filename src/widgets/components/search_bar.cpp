#include <QDebug>
#include "app_data_readonly.h"
#include "search_bar.h"
#include "services.h"
#include "ui_search_bar.h"
#include "utilities/numbers_util.h"
#include "widgets/app_style_sheet.h"
#include "widgets/icons.h"

SearchBar::SearchBar(QWidget *parent)
        : QFrame(parent)
        , ui(new Ui::SearchBar) {
    ui->setupUi(this);

    //
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    constexpr double fontSizeScaleFactor = 1.0;

    ui->iconLabel->setPixmap(getSearchIconPixmap(isDarkTheme, fontSizeScaleFactor));

    setStyleClasses(this, {StyleClass::frameWithSolidBorder});
    setStyleSheet(
            "SearchBar {"
            "  border-radius: 4px;"
            "  padding: 3px;"
            "}"
            "QLineEdit {"
            "  border: none;"
            "}"
    );

    //
    setUpConnections();
}

SearchBar::~SearchBar() {
    delete ui;
}

void SearchBar::setUpConnections() {
    connect(ui->lineEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        emit edited(text);
    });

    connect(ui->lineEdit, &QLineEdit::editingFinished, this, [this]() {
        emit editingFinished(ui->lineEdit->text());
    });

    //
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::fontSizeScaleFactorChanged,
            this, [this](const QWidget *window, const double factor) {
        if (this->window() != window)
            return;

        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        ui->iconLabel->setPixmap(getSearchIconPixmap(isDarkTheme, factor));
    });

    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {
        const double fontSizeScaleFactor
                = Services::instance()->getAppDataReadonly()->getFontSizeScaleFactor(this->window());
        ui->iconLabel->setPixmap(getSearchIconPixmap(isDarkTheme, fontSizeScaleFactor));
    });
}

QPixmap SearchBar::getSearchIconPixmap(const bool isDarkTheme, const double fontSizeScaleFactor) {
    const int iconSize = nearestInteger(18.0 * fontSizeScaleFactor);
    return Icons::getPixmap(
            Icon::Search,
            isDarkTheme ? Icons::Theme::Dark : Icons::Theme::Light,
            iconSize
    );
}
