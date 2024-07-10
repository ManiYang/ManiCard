#ifndef NODERECT_H
#define NODERECT_H

#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include "widgets/components/graphics_text_item.h"
#include "widgets/components/text_edit.h"

class GraphicsItemMoveResize;

class NodeRect : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit NodeRect(QGraphicsItem *parent = nullptr);

    //!
    //! Call this after this item is added to a scene.
    //!
    void initialize();

    // call these methods only after this item is added to a scene:
    void setRect(const QRectF rect_);
    void setColor(const QColor color_);
    void setBorderWidth(const double width);
    void setNodeLabel(const QString &label);
    void setCardId(const int cardId_);
    void setTitle(const QString &title_);
    void setText(const QString &text_);

    void setEditable(const bool editable);

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void movedOrResized();

private:
    QSizeF minSizeForResizing {100, 60};

    QRectF enclosingRect {QPointF(0, 0), QSizeF(90, 150)};
    QColor color {128, 128, 128};
    double borderWidth {6.0};
    QString nodeLabel;
    int cardId {-1};
    QString title;
    QString text;

    bool isEditable {true};

    // child items
    QGraphicsRectItem *captionBarItem; // also serves as move handle
    QGraphicsSimpleTextItem *nodeLabelItem;
    QGraphicsSimpleTextItem *cardIdItem;
    QGraphicsRectItem *contentsRectItem;
    // -- title
    GraphicsTextItem *titleItem;
    bool handleTitleItemContentChanged {true};
    // -- text
    //    Use QTextEdit (rather than QGraphicsTextItem, which does not have scrolling
    //    functionality)
    TextEdit *textEdit;
    QGraphicsProxyWidget *textEditProxyWidget;

    GraphicsItemMoveResize *moveResizeHelper;

    //
    void setUpConnections();

    void redraw() {
        update(); // re-paint
        adjustChildItems();
    }
    void adjustChildItems(const bool setTitleText = true);

};

#endif // NODERECT_H
