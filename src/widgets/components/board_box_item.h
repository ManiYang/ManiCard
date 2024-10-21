#ifndef BOARDBOXITEM_H
#define BOARDBOXITEM_H

#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QMenu>
#include <QRectF>

class GraphicsItemMoveResize;

class BoardBoxItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit BoardBoxItem(QGraphicsItem *parent = nullptr);
    ~BoardBoxItem();

    //!
    //! Call this after this item is added to the scene.
    //!
    void initialize();

    // Call these "set" methods only after this item is initialized:
    void setRect(const QRectF &rect);
    void setMarginWidth(const double width);
    void setBorderWidth(const double width);
    void setColor(const QColor &color_);
    void setIsHighlighted(const bool isHighlighted_);

    //
    QRectF getRect() const;
    bool getIsHighlighted() const;

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

signals:
    void clicked();
    void movedOrResized();
    void finishedMovingOrResizing();

protected:
    void setCaptionBarLeftText(const QString &text);
    void setCaptionBarLeftText(const QString &text, const bool bold);
    void setCaptionBarRightText(const QString &text);
    void setCaptionBarRightText(const QString &text, const bool bold);

    QRectF getContentsRect() const;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    // state variables
    QRectF borderOuterRect {0.0, 0.0, 100.0, 100.0};
    double marginWidth {2.0};
    double borderWidth {5.0};
    QColor color {160, 160, 160};
    bool isHighlighted {false};

    QRectF captionBarRect;
    double captionBarFontHeight;

    const double highlightBoxWidth {3.0};
    const QSizeF minSizeForResizing {100, 60};
    const int captionBarPadding {2};

    // child items
    QGraphicsRectItem *captionBarItem; // also serves as move handle
    QGraphicsSimpleTextItem *captionBarLeftTextItem;
    QGraphicsSimpleTextItem *captionBarRightTextItem;
    QGraphicsRectItem *contentsRectItem;

    GraphicsItemMoveResize *moveResizeHelper;
    QMenu *captionBarContextMenu {nullptr};

    //
    void setUpConnections();
    void installEventFilterOnChildItems();

    void setUpGraphicsItems();
    void adjustGraphicsItems();

    //!
    //! Call this when
    //! 1. `captionBarRightTextItem`'s text or font changes
    //! 2. one of the parameter values changes
    //!
    void setCaptionBarRightTextItemPos(const QRectF captionBarRect, const double captionBarPadding);

    // tools
    static QColor getHighlightBoxColor(const QColor &color);

    // ==== private virtual methods ====

    //!
    //! The returned \c QMenu can have no parent. Return \c nullptr if a context menu is not needed.
    //!
    virtual QMenu *createCaptionBarContextMenu();

    virtual void setUpContents(QGraphicsItem *contentsContainer);
    virtual void adjustContents();
};

#endif // BOARDBOXITEM_H
