#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H

#include <QMessageBox>

QMessageBox *createInformationMessageBox(QWidget *parent, const QString &title, const QString &text);
QMessageBox *createWarningMessageBox(QWidget *parent, const QString &title, const QString &text);

#endif // MESSAGE_BOX_H
