#include <QListWidgetItem>
#include "dialog_settings.h"
#include "ui_dialog_settings.h"
#include "widgets/dialogs/settings/appearance_settings.h"

DialogSettings::DialogSettings(QWidget *parent)
        : QDialog(parent)
        , ui(new Ui::DialogSettings) {
    ui->setupUi(this);
    setUpWidgets();
    setUpConnections();
}

DialogSettings::~DialogSettings() {
    delete ui;
}

void DialogSettings::setUpWidgets() {
    setWindowTitle("Settings");

    // remove ui->pageTest from ui->stackedWidget
    ui->stackedWidget->removeWidget(ui->pageTest);

    // add sections (the section names must be unique)
    addSection("Appearance", new AppearanceSettings);
    addSection("Test", ui->pageTest);
}

void DialogSettings::setUpConnections() {
    connect(ui->listWidgetSections, &QListWidget::currentTextChanged,
            this, [this](const QString &text) {
        for (const auto &callback: qAsConst(sectionSelectedCallbacks)) {
            callback(text);
        }
    });
}

void DialogSettings::addSection(const QString &name, QWidget *page) {
    ui->listWidgetSections->addItem(name);
    ui->stackedWidget->addWidget(page);
    sectionSelectedCallbacks.append([=](QString sectionName) {
        if (sectionName == name)
            ui->stackedWidget->setCurrentWidget(page);
    });
}
