#ifndef NODERECT_H
#define NODERECT_H

#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QSet>
#include "widgets/components/board_box_item.h"
#include "widgets/icons.h"

class CustomGraphicsTextItem;
class CustomTextEdit;

class NodeRect : public BoardBoxItem
{
    Q_OBJECT
public:
    explicit NodeRect(const int cardId, QGraphicsItem *parent = nullptr);
    ~NodeRect();

    // Call these "set" methods only after this item is initialized:
    void setNodeLabels(const QStringList &labels);
    void setTitle(const QString &title);
    void setPropertiesDisplay(const QString &propertiesDisplayText);
    void setText(const QString &text);
    void setEditable(const bool editable);

    void setTextEditorIgnoreWheelEvent(const bool b);

    void togglePreview();

    //
    int getCardId() const;
    QSet<QString> getNodeLabels() const;
    QString getTitle() const;
    QString getText() const;

signals:
    void leftButtonPressedOrClicked();
    void ctrlLeftButtonPressedOnCaptionBar();

    void titleTextUpdated(
            const std::optional<QString> &updatedTitle,
            const std::optional<QString> &updatedText);
    void userToSetLabels();
    void userToCreateRelationship();
    void closeByUser();

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    static BoardBoxItem::CreationParameters getCreationParameters();

    const int cardId;
    const double textEditFocusIndicatorLineWidth {2.0};
    QStringList nodeLabels;
    bool textEditIgnoreWheelEvent {false};
    bool nodeRectIsEditable {false};

    QString plainText;
    bool textEditIsPreviewMode {false};
    int textEditCursorPositionBeforePreviewMode {0};

    QHash<QAction *, Icon> contextMenuActionToIcon;

    // content items
    // -- title
    CustomGraphicsTextItem *titleItem;
    // -- properties
    CustomGraphicsTextItem *propertiesItem;
    // -- text (Use QTextEdit rather than QGraphicsTextItem. The latter does not have scrolling
    //    functionality.)
    CustomTextEdit *textEdit;
    QGraphicsProxyWidget *textEditProxyWidget;
    QGraphicsRectItem *textEditFocusIndicator;

    // override
    QMenu *createCaptionBarContextMenu() override;
    void adjustCaptionBarContextMenuBeforePopup(QMenu *contextMenu) override;
    void setUpContents(QGraphicsItem *contentsContainer) override;
    void adjustContents() override;
    void onMouseLeftPressed(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;
    void onMouseLeftClicked(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;

    // tools
    static QString getNodeLabelsString(const QStringList &labels);
    static bool computeTextEditEditable(
            const bool nodeRectIsEditable, const bool textEditIsPreviewMode);
    static QColor getNormalTextColor(const bool isDarkTheme);
    static QColor getDimTextColor(const bool isDarkTheme);
    static QPen getTextEditFocusIndicator(const bool isDarkTheme, const double indicatorLineWidth);
};

#endif // NODERECT_H
