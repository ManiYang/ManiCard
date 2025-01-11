#ifndef DIALOG_WORKSPACE_CARD_COLORS_H
#define DIALOG_WORKSPACE_CARD_COLORS_H

#include <QBoxLayout>
#include <QDialog>
#include <QLabel>

namespace Ui {
class DialogWorkspaceCardColors;
}

class ColorDisplayWidget;

class DialogWorkspaceCardColors : public QDialog
{
    Q_OBJECT

public:
    using LabelAndColor = std::pair<QString, QColor>;

    explicit DialogWorkspaceCardColors(
            const QString &workspaceName,
            const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
            const QColor &defaultNodeRectColor, QWidget *parent = nullptr);
    ~DialogWorkspaceCardColors();

    QVector<LabelAndColor> getCardLabelsAndAssociatedColors() const;
    QColor getDefaultColor() const;

private:
    Ui::DialogWorkspaceCardColors *ui;

    void setUpWidgets(
            const QString &workspaceName,
            const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
            const QColor &defaultNodeRectColor);
    void setUpConnections();

    //
    void onUserToPickColorForRow(const int row);
    void onUserToRemoveRow(const int row, const QString &cardLabel);

    void addRowToLabelColorAssociationTable(const QString &label, const QColor &color);
    void swapRows(const int row1, const int row2); // precedence cells are not swapped
    void updatePrecedenceNumbers();
    void validateLabels();

    void setDefaultColor(const QColor &color);

    //!
    //! \return -1 if not found
    //!
    int getRowOfColorDisplayWidget(ColorDisplayWidget *colorDisplayWidget) const;
};

//====

class ColorDisplayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ColorDisplayWidget(QWidget *parent = nullptr);

    void setColor(const QColor &color);
    QColor getColor() const;

signals:
    void doubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *labelColorSample {nullptr};
    QLabel *labelColorHex {nullptr};
};

#endif // DIALOG_WORKSPACE_CARD_COLORS_H
