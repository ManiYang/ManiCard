#ifndef NODERECT_H
#define NODERECT_H

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

class NodeRect : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit NodeRect(const int cardId, QGraphicsItem *parent = nullptr);
    ~NodeRect();

    //!
    //! Call this after this item is added to a scene.
    //!
    void initialize();

    //
    enum class InputVar {
        Rect=0, // QRectF
        Color, // QColor
        MarginWidth, // double
        BorderWidth, // double
        NodeLabels, // QStringList
        IsEditable, // bool
        IsHighlighted, // bool
        _Count
    };

    // Call these "set" methods only after this item is initialized:
    void set(const QMap<InputVar, QVariant> &values);
    void setTitle(const QString &title);
    void setText(const QString &text);

    //
    int getCardId() const;
    QRectF getRect() const;
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
    const double highlightBoxWidth {3.0};

    enum class DependentVar {
        HighlightBoxColor=0, // QColor
        _Count
    };

    VariablesUpdatePropagator<InputVar, DependentVar> vars;

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
    void setUpContextMenu();
    void setUpConnections();
    void installEventFilterOnChildItems();

    //
    void updateInputVars(const QMap<InputVar, QVariant> &values);
    void adjustChildItems();

    // tools
    QGraphicsView *getView() const; // can be nullptr
    static QString getNodeLabelsString(const QStringList &labels);
    static QColor getHighlightBoxColor(const QColor &color);

    static QMap<InputVar, QString> inputVariableNames();
    static QMap<DependentVar, QString> dependentVariableNames();
};

#endif // NODERECT_H
