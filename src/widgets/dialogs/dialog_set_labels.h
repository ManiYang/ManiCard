#ifndef DIALOG_SET_LABELS_H
#define DIALOG_SET_LABELS_H

#include <QDialog>
#include <QHash>
#include <QListWidgetItem>
#include <QSet>

namespace Ui {
class DialogSetLabels;
}

class DialogSetLabels : public QDialog
{
    Q_OBJECT
public:
    explicit DialogSetLabels(
            const QSet<QString> &initialCardLabels, const QStringList &userDefinedLabelsList,
            QWidget *parent = nullptr);
    ~DialogSetLabels();

    QStringList getLabels() const;

private:
    Ui::DialogSetLabels *ui;

    const QStringList userDefinedLabelsList;
    QHash<QString, QListWidgetItem *> labelToLabelsListItem;
            // values are items of ui->listWidgetLabelsList

    void setUpLabelsList();
    void setUpConnections();

    void addLabelToCurrentCardLabels(const QString &label);
    void updateLabelsList(const QStringList &currentCardLabels);

    QStringList getCurrentCardLabels() const;
};

#endif // DIALOG_SET_LABELS_H
