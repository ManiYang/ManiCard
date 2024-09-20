#ifndef DIALOG_BOARD_CARD_COLORS_H
#define DIALOG_BOARD_CARD_COLORS_H

#include <QBoxLayout>
#include <QDialog>
#include <QLabel>
#include "models/board.h"

namespace Ui {
class DialogBoardCardColors;
}

class ColorDisplayWidget;

class DialogBoardCardColors : public QDialog
{
    Q_OBJECT

public:
    explicit DialogBoardCardColors(
            const QString &boardName,
            const QVector<Board::LabelAndColor> &cardLabelsAndAssociatedColors,
            const QColor &defaultNodeRectColor, QWidget *parent = nullptr);
    ~DialogBoardCardColors();

    QVector<Board::LabelAndColor> getCardLabelsAndAssociatedColors() const;
    QColor getDefaultColor() const;

private:
    Ui::DialogBoardCardColors *ui;

    void setUpWidgets(
            const QString &boardName,
            const QVector<Board::LabelAndColor> &cardLabelsAndAssociatedColors,
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

#endif // DIALOG_BOARD_CARD_COLORS_H
