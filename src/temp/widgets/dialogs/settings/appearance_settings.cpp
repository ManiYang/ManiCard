#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QFontDialog>
#include "appearance_settings.h"
#include "ui_appearance_settings.h"

AppearanceSettings::AppearanceSettings(QWidget *parent)
        : QFrame(parent)
        , ui(new Ui::AppearanceSettings) {
    ui->setupUi(this);
    setUpConnections();
}

AppearanceSettings::~AppearanceSettings() {
    delete ui;
}

void AppearanceSettings::setUpConnections() {
    connect(ui->buttonSelectFont, &QPushButton::clicked, this, [this]() {
//        auto *dialog = new QFontDialog(font(), this);
//        connect(dialog, &QDialog::finished, this, [dialog](int result) {
//            if (result == QDialog::Accepted) {
//                qApp->setFont(dialog->currentFont()); // dialog->selectedFont() won't work
//            }
//            dialog->deleteLater();
//        });
//        dialog->open();
    });
}
