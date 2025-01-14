#ifndef BOARDBOXITEM_H
#define BOARDBOXITEM_H

#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QMenu>
#include <QRectF>

class GraphicsItemMoveResize;

class BoardBoxItem : public QGraphicsObject
{
    Q_OBJECT
public:
    enum class ContentsBackgroundType {
        White,
        Transparent //!< the contents rect will be transparent and will not intercept mouse events
    };

    enum class BorderShape {Solid, Dashed};

    struct CreationParameters
    {
        ContentsBackgroundType contentsBackgroundType {ContentsBackgroundType::White};
        BorderShape borderShape {BorderShape::Solid};
        QColor highlightFrameColor {36, 128, 220};
    };

public:
    explicit BoardBoxItem(const CreationParameters &parameters, QGraphicsItem *parent = nullptr);
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
    QRectF getContentsRect() const;

    //
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

signals:
//    void mousePressedOnCaptionBar();
//    void clicked();
    void aboutToMove();
    void movedOrResized();
    void finishedMovingOrResizing();

protected:
    void setCaptionBarLeftText(const QString &text);
    void setCaptionBarLeftText(const QString &text, const bool bold);
    void setCaptionBarRightText(const QString &text);
    void setCaptionBarRightText(const QString &text, const bool bold);

    QGraphicsView *getView() const; // can return nullptr

    //
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    // state variables
    const BorderShape borderShape;
    const ContentsBackgroundType contentsBackgroundType;
    const QColor highlightFrameColor;

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
    QGraphicsRectItem *captionBarMatItem; // before `captionBarItem`
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

    //
//    static QColor getHighlightBoxColor(const QColor &color);

    // ==== private virtual methods ====

    //!
    //! The returned \c QMenu can have no parent. Return \c nullptr if a context menu is not needed.
    //!
    virtual QMenu *createCaptionBarContextMenu();

    virtual void setUpContents(QGraphicsItem *contentsContainer);
    virtual void adjustContents();

    virtual void onMousePressedOnCaptionBar();
    virtual void onMouseClicked();
};

#endif // BOARDBOXITEM_H
