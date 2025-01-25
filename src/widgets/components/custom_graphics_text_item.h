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
    void setTextSelectable(const bool selectable);
    void setTextWidth(const double width);
    void setFont(const QFont &font);
    void setDefaultTextColor(const QColor &color);
    void setEnableContextMenu(const bool enable);

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
    void tabKeyPressed();

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

    void setEnableContextMenu(const bool enable);

    //
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

    bool event(QEvent *event) override;

signals:
    void mouseReleased();
    void tabKeyPressed();

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    bool enableContextMenu {false};
};

#endif // GRAPHICSTEXTITEM_H
