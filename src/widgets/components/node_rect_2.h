#ifndef NODERECT2_H
#define NODERECT2_H

#include <optional>
#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QMenu>
#include <QSet>
#include "utilities/variables_update_propagator.h"

class CustomTextEdit;
class CustomGraphicsTextItem;
class GraphicsItemMoveResize;

class NodeRect2 : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit NodeRect2(const int cardId, QGraphicsItem *parent = nullptr);
    ~NodeRect2();

    //!
    //! Call this after this item is added to a scene.
    //!
    void initialize();

    //

    enum class Var {
        Rect, // QRectF
        Color, // QColor
        MarginWidht, // double
        BorderWidth, // double
        NodeLabels, // QStringList
        Title, // QString
        Text, // QString
        IsEditable, // bool
        IsHighlighted, // bool
    };



    // Call these "set" methods only after this item is initialized:

//    void setRect(const QRectF rect_);
//    void setColor(const QColor color_);
//    void setMarginWidth(const double width);
//    void setBorderWidth(const double width);

//    void setNodeLabels(const QStringList &labels);
//    void setNodeLabels(const QVector<QString> &labels);
//    void setTitle(const QString &title);
//    void setText(const QString &text);

//    void setEditable(const bool editable);
//    void setHighlighted(const bool highlighted);

    //
    QRectF getRect() const;

    int getCardId() const;
    QSet<QString> getNodeLabels() const;
    QString getTitle() const;
    QString getText() const;
    bool getIsHighlighted() const;

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

signals:
    void movedOrResized();
    void finishedMovingOrResizing();

    void clicked();

    void titleTextUpdated(
            const std::optional<QString> &updatedTitle,
            const std::optional<QString> &updatedText);

    void userToSetLabels();
    void userToCreateRelationship();
    void closeByUser();

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    const int cardId;
    const QSizeF minSizeForResizing {100, 60}; // (pixel)
    const double textEditLineHeightPercent {120};

    VariablesUpdatePropagator<Var> varsUpdatePropagator;

//    QRectF enclosingRect {QPointF(0, 0), QSizeF(90, 150)};
//    QColor color {160, 160, 160};
//    double marginWidth {2.0};
//    double borderWidth {5.0};
//    QStringList nodeLabels;
//    bool isEditable {true};
//    bool isHighlighted {false};
//    const double highlightBoxWidth {3.0};

    // child items
    QGraphicsRectItem *captionBarItem; // also serves as move handle
    QGraphicsSimpleTextItem *nodeLabelItem;
    QGraphicsSimpleTextItem *cardIdItem;
    QGraphicsRectItem *contentsRectItem;
    // -- title
    CustomGraphicsTextItem *titleItem;
    // -- text (Use QTextEdit rather than QGraphicsTextItem. The latter does not have scrolling
    //    functionality.)
    CustomTextEdit *textEdit;
    QGraphicsProxyWidget *textEditProxyWidget;
    QGraphicsRectItem *textEditFocusIndicator;

    //
    GraphicsItemMoveResize *moveResizeHelper;
    QMenu *contextMenu;

    //
    void installEventFilterOnChildItems();
    void setUpContextMenu();
    void setUpConnections();

    void redraw() {
        update(); // re-paint
        adjustChildItems();
    }
    void adjustChildItems();

    // tools
    QGraphicsView *getView() const; // can be nullptr
    static QString getNodeLabelsString(const QStringList &labels);
    static QColor getHighlightBoxColor(const QColor &color);
};

#endif // NODERECT2_H
