#ifndef NODERECT_H
#define NODERECT_H

#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QSet>
#include "widgets/components/board_box_item.h"

class CustomGraphicsTextItem;
class CustomTextEdit;

class NodeRect : public BoardBoxItem
{
    Q_OBJECT
public:
    explicit NodeRect(const int cardId, QGraphicsItem *parent = nullptr);

    // Call these "set" methods only after this item is initialized:
    void setNodeLabels(const QStringList &labels);
    void setTitle(const QString &title);
    void setText(const QString &text);
    void setEditable(const bool editable);

    void setTextEditorIgnoreWheelEvent(const bool b);

    //
    int getCardId() const;
    QSet<QString> getNodeLabels() const;
    QString getTitle() const;
    QString getText() const;

signals:
    void mousePressedOrClicked();
    void titleTextUpdated(
            const std::optional<QString> &updatedTitle,
            const std::optional<QString> &updatedText);
    void userToSetLabels();
    void userToCreateRelationship();
    void closeByUser();

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    const int cardId;
    const double textEditFocusIndicatorLineWidth {2.0};
    QStringList nodeLabels;
    bool textEditIgnoreWheelEvent {false};

    // content items
    // -- title
    CustomGraphicsTextItem *titleItem;
    // -- text (Use QTextEdit rather than QGraphicsTextItem. The latter does not have scrolling
    //    functionality.)
    CustomTextEdit *textEdit;
    QGraphicsProxyWidget *textEditProxyWidget;
    QGraphicsRectItem *textEditFocusIndicator;

    // override
    QMenu *createCaptionBarContextMenu() override;
    void setUpContents(QGraphicsItem *contentsContainer) override;
    void adjustContents() override;
    void onMousePressed(const bool isOnCaptionBar) override;
    void onMouseLeftClicked() override;

    // tools
    static QString getNodeLabelsString(const QStringList &labels);
};

#endif // NODERECT_H
