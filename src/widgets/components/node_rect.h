#ifndef NODERECT_H
#define NODERECT_H

#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QSet>
#include "utilities/saving_debouncer.h"
#include "widgets/components/graphics_text_item.h"

class CustomTextEdit;
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

    // Call these methods only after this item is added to a scene:

    void setRect(const QRectF rect_);
    void setColor(const QColor color_);
    void setMarginWidth(const double width);
    void setBorderWidth(const double width);

    void setNodeLabels(const QSet<QString> &labels);
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

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    QSizeF minSizeForResizing {100, 60};

    QRectF enclosingRect {QPointF(0, 0), QSizeF(90, 150)};
    QColor color {160, 160, 160};
    double marginWidth {1.0};
    double borderWidth {5.0};
    QSet<QString> nodeLabels;
    int cardId {-1};
//    QString text;

    bool isEditable {true};

    // child items
    QGraphicsRectItem *captionBarItem; // also serves as move handle
    QGraphicsSimpleTextItem *nodeLabelItem;
    QGraphicsSimpleTextItem *cardIdItem;
    QGraphicsRectItem *contentsRectItem;
    // -- title
    GraphicsTextItem *titleItem;
    bool handleTitleItemContentChanged {true}; // <----------- use subclass
    // -- text
    //    Use QTextEdit rather than QGraphicsTextItem. The latter does not have scrolling
    //    functionality.
    CustomTextEdit *textEdit;
//    bool handleTextEditTextChanged {true}; // <----------- use subclass
    QGraphicsProxyWidget *textEditProxyWidget;

    GraphicsItemMoveResize *moveResizeHelper;

    //
    SavingDebouncer *propertiesSaving;
    bool titleEdited {false};
    bool textEdited {false};

    //
    void setUpConnections();

    void redraw() {
        update(); // re-paint
        adjustChildItems();
    }
    void adjustChildItems();

    static QString getNodeLabelsString(const QSet<QString> &labels);
};

#endif // NODERECT_H
