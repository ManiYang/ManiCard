#ifndef APPEARANCE_SETTINGS_H
#define APPEARANCE_SETTINGS_H

#include <QFrame>

namespace Ui {
class AppearanceSettings;
}

class AppearanceSettings : public QFrame
{
    Q_OBJECT

public:
    explicit AppearanceSettings(QWidget *parent = nullptr);
    ~AppearanceSettings();

private:
    Ui::AppearanceSettings *ui;

    void setUpConnections();
};

#endif // APPEARANCE_SETTINGS_H
