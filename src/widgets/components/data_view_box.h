#ifndef DATAVIEWBOX_H
#define DATAVIEWBOX_H

#include <QGraphicsProxyWidget>
#include "widgets/components/board_box_item.h"
#include "widgets/components/custom_graphics_text_item.h"
#include "widgets/components/custom_text_edit.h"

class DataViewBox : public BoardBoxItem
{
    Q_OBJECT
public:
    explicit DataViewBox(const int dataQueryId, QGraphicsItem *parent = nullptr);

    // Call these "set" methods only after this item is initialized:
    void setTitle(const QString &title);
    void setQuery(const QString &query);
    void setEditable(const bool editable);

    void setTextEditorIgnoreWheelEvent(const bool b);

    //
    int getDataQueryId() const;

signals:
    void titleUpdated(const QString &updatedTitle);
    void queryUpdated(const QString &updatedQuery);

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    const int dataQueryId;
    bool textEditIgnoreWheelEvent {false};

    // content items
    // -- title
    CustomGraphicsTextItem *titleItem;
    // -- query
    CustomGraphicsTextItem *queryItem;
    // -- text (Use QTextEdit rather than QGraphicsTextItem. The latter does not have scrolling
    //    functionality.)
    CustomTextEdit *textEdit; // shows query result
    QGraphicsProxyWidget *textEditProxyWidget;

    //
    QMenu *createCaptionBarContextMenu() override;
    void setUpContents(QGraphicsItem *contentsContainer) override;
    void adjustContents() override;
};

#endif // DATAVIEWBOX_H