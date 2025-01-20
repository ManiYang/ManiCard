#include "app_data.h"
#include "app_data_readonly.h"
#include "dialog_options.h"
#include "services.h"
#include "ui_dialog_options.h"

DialogOptions::DialogOptions(QWidget *parent)
        : QDialog(parent)
        , ui(new Ui::DialogOptions) {
    ui->setupUi(this);

    setWindowTitle("Options");

    setStyleSheet(
            "QDialog QFrame {"
            "  background: none;"
            "}"
            "* {"
            "  font-size: 11pt;"
            "}");

    setUpWidgets();
}

DialogOptions::~DialogOptions() {
    delete ui;
}

void DialogOptions::setUpWidgets() {
    //
    ui->checkBoxDarkTheme->setChecked(
            Services::instance()->getAppDataReadonly()->getIsDarkTheme());
    connect(ui->checkBoxDarkTheme, &QCheckBox::clicked, this, [this](bool checked) {
        Services::instance()->getAppData()->updateIsDarkTheme(EventSource(this), checked);
    });

    //
    ui->checkBoxAutoAdjustCardColors->setChecked(
            Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme());
    connect(ui->checkBoxAutoAdjustCardColors, &QCheckBox::clicked, this, [this](bool checked) {
        Services::instance()->getAppData()->updateAutoAdjustCardColorsForDarkTheme(
                EventSource(this), checked);
    });
}
