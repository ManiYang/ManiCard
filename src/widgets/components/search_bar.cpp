#include <QDebug>
#include <QKeyEvent>
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

    ui->iconLabel->setPixmap(getSearchIconPixmap(isDarkTheme, fontSize, fontSizeScaleFactor));

    ui->lineEdit->installEventFilter(this);

    setFocusProxy(ui->lineEdit);

    // styles
    setStyleClasses(this, {StyleClass::frameWithSolidBorder});
    QFrame::setStyleSheet(
            "SearchBar {"
            "  border-radius: 4px;"
            "  padding: 3px;"
            "}"
    );

    getStyleSheetForLineEdit = [](const double fontPointSize) {
        return QString() +
                "QLineEdit {"
                "  border: none;"
                "  font-size: " + QString::number(fontPointSize, 'f', 1) + "pt;"
                "}";
    };
    ui->lineEdit->setStyleSheet(getStyleSheetForLineEdit(fontSize));

    //
    setUpConnections();
}

SearchBar::~SearchBar() {
    delete ui;
}

void SearchBar::setPlaceholderText(const QString &text) {
    ui->lineEdit->setPlaceholderText(text);
}

void SearchBar::setFontPointSize(const double fontPointSize) {
    fontSize = fontPointSize;

    //
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    const double fontSizeScaleFactor
            = Services::instance()->getAppDataReadonly()->getFontSizeScaleFactor(this->window());
    ui->iconLabel->setPixmap(getSearchIconPixmap(isDarkTheme, fontSize, fontSizeScaleFactor));

    //
    ui->lineEdit->setStyleSheet(getStyleSheetForLineEdit(fontSize));
}

bool SearchBar::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->lineEdit) {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
            Q_ASSERT(keyEvent != nullptr);
            if (!keyEvent->isAutoRepeat()) {
                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
                    emit submitted(ui->lineEdit->text());
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void SearchBar::setUpConnections() {
    connect(ui->lineEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        emit edited(text);
    });

    //
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::fontSizeScaleFactorChanged,
            this, [this](const QWidget *window, const double factor) {
        if (this->window() != window)
            return;

        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        ui->iconLabel->setPixmap(getSearchIconPixmap(isDarkTheme, fontSize, factor));
    });

    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {
        const double fontSizeScaleFactor
                = Services::instance()->getAppDataReadonly()->getFontSizeScaleFactor(this->window());
        ui->iconLabel->setPixmap(getSearchIconPixmap(isDarkTheme, fontSize, fontSizeScaleFactor));
    });
}

QPixmap SearchBar::getSearchIconPixmap(
        const bool isDarkTheme, const double fontPointSize, const double fontSizeScaleFactor) {
    const int iconSize = nearestInteger(fontPointSize * 1.8 * fontSizeScaleFactor);
    return Icons::getPixmap(
            Icon::Search,
            isDarkTheme ? Icons::Theme::Dark : Icons::Theme::Light,
            iconSize
    );
}
