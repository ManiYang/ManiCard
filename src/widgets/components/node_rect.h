#ifndef NODERECT_H
#define NODERECT_H

#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsObject>
#include <QGraphicsSimpleTextItem>

class NodeRect : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit NodeRect(QGraphicsItem *parent = nullptr);

    // add this item to scene before setting the parameters
    void setRect(const QRectF rect_);
    void setColor(const QColor color_);
    void setBorderWidth(const double width);
    void setNodeLabel(const QString &label);
    void setCardId(const int cardId_);

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QRectF enclosingRect {QPointF(0, 0), QSizeF(90, 150)};
    QColor color {128, 128, 128};
    double borderWidth {5.0};
    QString nodeLabel;
    int cardId {-1};

    // child items
    QGraphicsRectItem *captionBarItem;
    QGraphicsSimpleTextItem *nodeLabelItem;
    QGraphicsSimpleTextItem *cardIdItem;

    //
    void redraw() {
        update(); // re-paint
        adjustChildItems();
    }
    void adjustChildItems();

};

#endif // NODERECT_H
