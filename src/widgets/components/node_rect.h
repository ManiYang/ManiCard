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
#include "utilities/save_debouncer.h"

using StringOpt = std::optional<QString>;

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

    // Call these "set" methods only after this item is added to a scene:

    void setRect(const QRectF rect_);
    void setColor(const QColor color_);
    void setMarginWidth(const double width);
    void setBorderWidth(const double width);

    void setNodeLabels(const QStringList &labels);
    void setNodeLabels(const QVector<QString> &labels);
    void setTitle(const QString &title);
    void setText(const QString &text);

    void setEditable(const bool editable);

    //
    void finishedSaveTitleText(); // cf. signal saveTitleTextUpdate()
    void prepareToClose(); // cf. canClose()

    //
    QRectF getRect() const;

    int getCardId() const;
    QSet<QString> getNodeLabels() const;
    QString getTitle() const;
    QString getText() const;

    bool canClose() const;

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget) override;

signals:
    void movedOrResized();
    void finishedMovingOrResizing();

    //!
    //! The receiver should start saving the properties update, and call
    //! \c finishedSaveTitleText() when finished saving.
    //!
    void saveTitleTextUpdate(const StringOpt &updatedTitle, const StringOpt &updatedText);

    void userToSetLabels();
    void userToCreateRelationship();
    void closeByUser();

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    QSizeF minSizeForResizing {100, 60};

    QRectF enclosingRect {QPointF(0, 0), QSizeF(90, 150)};
    QColor color {160, 160, 160};
    double marginWidth {1.0};
    double borderWidth {5.0};
    QStringList nodeLabels;
    const int cardId;

    bool isEditable {true};

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

    //
    GraphicsItemMoveResize *moveResizeHelper;
    QMenu *contextMenu;

    SaveDebouncer *titleTextSaveDebouncer;
    bool titleEdited {false};
    bool textEdited {false};
    constexpr static int titleTextSaveDelay {3000}; // (ms)

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
    QGraphicsView *getView(); // can be nullptr
    static QString getNodeLabelsString(const QStringList &labels);
};

#endif // NODERECT_H
