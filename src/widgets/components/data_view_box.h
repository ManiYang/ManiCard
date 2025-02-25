#ifndef DATAVIEWBOX_H
#define DATAVIEWBOX_H

#include <QGraphicsProxyWidget>
#include "widgets/components/board_box_item.h"
#include "widgets/components/custom_graphics_text_item.h"
#include "widgets/components/custom_text_edit.h"
#include "widgets/icons.h"

class DataViewBox : public BoardBoxItem
{
    Q_OBJECT
public:
    explicit DataViewBox(const int customDataQueryId, QGraphicsItem *parent = nullptr);
    ~DataViewBox();

    // Call these "set" methods only after this item is initialized:
    void setTitle(const QString &title);
    void setQuery(const QString &cypher, const QJsonObject &parameters);
    void setEditable(const bool editable);

    void setTextEditorIgnoreWheelEvent(const bool b);

    //
    int getCustomDataQueryId() const;

signals:
    void leftButtonPressedOrClicked();
    void titleUpdated(const QString &updatedTitle);
    void queryUpdated(const QString &cypher, const QJsonObject &parameters);
    void closeByUser();

    void getCardIdsOfBoard(QSet<int> *cardIds);

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    const int customDataQueryId;
    bool textEditIgnoreWheelEvent {false};

    enum class ResultDisplayFormat { JsonObjects, MarkdownTable };
    ResultDisplayFormat resultDisplayFormat {ResultDisplayFormat::JsonObjects};

    QHash<QAction *, Icon> contextMenuActionToIcon;

    // content items
    // -- title
    CustomGraphicsTextItem *titleItem;

    // -- queryCypher
    QGraphicsSimpleTextItem *labelCypher;
    CustomGraphicsTextItem *queryCypherItem;
    CustomGraphicsTextItem *queryCypherErrorMsgItem;

    // -- queryParameters
    QGraphicsSimpleTextItem *labelParameters;
    CustomGraphicsTextItem *queryParametersItem;
    CustomGraphicsTextItem *queryParamsErrorMsgItem;

    // -- query result
    QGraphicsSimpleTextItem *labelQueryResult;
    CustomTextEdit *textEdit; // shows query result
            // (Use QTextEdit rather than QGraphicsTextItem. The latter does not have scrolling
            // functionality.)
    QGraphicsProxyWidget *textEditProxyWidget;

    // override
    QMenu *createCaptionBarContextMenu() override;
    void adjustCaptionBarContextMenuBeforePopup(QMenu *contextMenu) override;
    void setUpContents(QGraphicsItem *contentsContainer) override;
    void adjustContents() override;
    void onMouseLeftPressed(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;
    void onMouseLeftClicked(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;

    //
    void setResultDisplayFormat(const ResultDisplayFormat format);

    //!
    //! Updates the text of `queryCypherErrorMsgItem`.
    //! \return (is-validation-OK?, is-error-msg-changed?)
    //!
    std::pair<bool, bool> validateQueryCypher();

    //!
    //! Updates the text of `queryParamsErrorMsgItem`.
    //! \return (is-validation-OK?, is-error-msg-changed?)
    //!
    std::pair<bool, bool> validateQueryParameters();

    void runQuery(std::function<void (bool ok, const QVector<QJsonObject> &result)> callback);
    void exportQueryResult(const QVector<QJsonObject> &queryResult) const;

    static QColor getTitleItemDefaultTextColor(const bool isDarkTheme);
    static QString getResultDisplay(
            const QVector<QJsonObject> &resultRows, const ResultDisplayFormat format);
    static QString printJsonValue(const QJsonValue &v);
};

#endif // DATAVIEWBOX_H
