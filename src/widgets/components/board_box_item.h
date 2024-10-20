#ifndef BOARDBOXITEM_H
#define BOARDBOXITEM_H

#include <QGraphicsObject>

class BoardBoxItem : public QGraphicsObject
{
public:
    explicit BoardBoxItem(QGraphicsItem *parent = nullptr);

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

private:
    void setUpGraphicsItems();
};

#endif // BOARDBOXITEM_H
