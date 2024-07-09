#ifndef GRAPHICSTEXTITEM_H
#define GRAPHICSTEXTITEM_H

#include <QGraphicsTextItem>

class GraphicsTextItem : public QGraphicsTextItem
{
    Q_OBJECT
public:
    explicit GraphicsTextItem(QGraphicsItem *parent = nullptr);

    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void contentChanged(bool heightChanged);

protected:
    void focusOutEvent(QFocusEvent *event) override;

private:
    double height; // for detecting height change

    void onContentsChanged();
};

#endif // GRAPHICSTEXTITEM_H
