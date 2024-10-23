#ifndef GRAPHICSTEXTITEM_H
#define GRAPHICSTEXTITEM_H

#include <QGraphicsTextItem>

class GraphicsTextItemTweak;

//!
//! Wraps a QGraphicsTextItem.
//! Emits signal textEdited() only when the text is updated by user.
//!
class CustomGraphicsTextItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit CustomGraphicsTextItem(QGraphicsItem *parent = nullptr);

    void setPlainText(const QString &text);
    void setEditable(const bool editable);
    void setTextWidth(const double width);
    void setFont(const QFont &font);
    void setDefaultTextColor(const QColor &color);

    QString toPlainText() const;
    QFont font() const;

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

signals:
    void textEdited(bool heightChanged);
    void clicked();

private:
    GraphicsTextItemTweak *graphicsTextItem;
    bool textChangeIsByUser {true};
    double height;
};


class GraphicsTextItemTweak : public QGraphicsTextItem
{
    Q_OBJECT
public:
    explicit GraphicsTextItemTweak(QGraphicsItem *parent = nullptr);

    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

signals:
    void mouseReleased();

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // GRAPHICSTEXTITEM_H
