#ifndef DIALOG_OPTIONS_H
#define DIALOG_OPTIONS_H

#include <QDialog>

namespace Ui {
class DialogOptions;
}

class DialogOptions : public QDialog
{
    Q_OBJECT
public:
    explicit DialogOptions(QWidget *parent = nullptr);
    ~DialogOptions();

private:
    Ui::DialogOptions *ui;

    void setUpWidgets();
};

#endif // DIALOG_OPTIONS_H
