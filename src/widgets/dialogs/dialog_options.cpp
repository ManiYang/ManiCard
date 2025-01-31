#include <QFileDialog>
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

    //
    ui->lineEditExportOutputDir->setReadOnly(true);
    ui->lineEditExportOutputDir->setText(
            Services::instance()->getAppDataReadonly()->getExportOutputDir());

    connect(ui->buttonSelectExportOutputDir, &QPushButton::clicked, this, [this]() {
        const QString oldOutputDir = ui->lineEditExportOutputDir->text();

        const QString newOutputDir = QFileDialog::getExistingDirectory(
                this, "Select output directory", oldOutputDir);
        ui->lineEditExportOutputDir->setText(newOutputDir);

        Services::instance()->getAppData()->updateExportOutputDir(EventSource(this), newOutputDir);
    });

    //
    setStyleSheet(
            "QDialog QFrame {"
            "  background: none;"
            "}"
            "* {"
            "  font-size: 11pt;"
            "}"
            "#labelSectionAppearance, #labelSectionExport {"
            "  font-weight: bold;"
            "  font-size: 12pt;"
            "  margin-top: 8px;"
            "}");
}
