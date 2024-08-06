#include "message_box.h"

QMessageBox *createInformationMessageBox(
        QWidget *parent, const QString &title, const QString &text) {
    auto *messageBox = new QMessageBox(parent);
    messageBox->setIcon(QMessageBox::Information);
    messageBox->setWindowTitle(title);
    messageBox->setText(text);
    messageBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageBox->setStandardButtons(QMessageBox::Ok);
    return messageBox;
}

QMessageBox *createWarningMessageBox(QWidget *parent, const QString &title, const QString &text) {
    auto *messageBox = new QMessageBox(parent);
    messageBox->setWindowTitle(title);
    messageBox->setIcon(QMessageBox::Warning);
    messageBox->setText(text);
    messageBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageBox->setStandardButtons(QMessageBox::Ok);
    return messageBox;
}

void showInformationMessageBox(QWidget *parent, const QString &title, const QString &text) {
    auto *msgBox = createInformationMessageBox(parent, title, text);
    msgBox->exec();
    msgBox->deleteLater();
}

void showWarningMessageBox(QWidget *parent, const QString &title, const QString &text) {
    auto *msgBox = createWarningMessageBox(parent, title, text);
    msgBox->exec();
    msgBox->deleteLater();
}
