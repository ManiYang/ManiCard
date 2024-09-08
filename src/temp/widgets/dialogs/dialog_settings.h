#ifndef DIALOG_SETTINGS_H
#define DIALOG_SETTINGS_H

#include <functional>
#include <QDialog>

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(QWidget *parent = nullptr);
    ~DialogSettings();

private:
    Ui::DialogSettings *ui;

    QList<std::function<void (QString sectionName)>> sectionSelectedCallbacks;

    void setUpWidgets();
    void setUpConnections();

    // tools
    void addSection(const QString &name, QWidget *page);
};

#endif // DIALOG_SETTINGS_H
