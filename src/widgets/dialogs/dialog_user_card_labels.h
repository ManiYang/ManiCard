#ifndef DIALOG_USER_CARD_LABELS_H
#define DIALOG_USER_CARD_LABELS_H

#include <QDialog>

namespace Ui {
class DialogUserCardLabels;
}

//!
//! Lets user edit the list of user-defined node labels.
//!
class DialogUserCardLabels : public QDialog
{
    Q_OBJECT
public:
    explicit DialogUserCardLabels(const QStringList labels, QWidget *parent = nullptr);
    ~DialogUserCardLabels();

    QStringList getLabelsList() const;

private:
    Ui::DialogUserCardLabels *ui;

    void setUpConnections();
};

#endif // DIALOG_USER_CARD_LABELS_H
