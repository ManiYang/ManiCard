#include <QApplication>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMessageBox>
#include "app_data_readonly.h"
#include "data_view_box.h"
#include "models/custom_data_query.h"
#include "services.h"
#include "utilities/json_util.h"

DataViewBox::DataViewBox(const int customDataQueryId, QGraphicsItem *parent)
        : BoardBoxItem(BoardBoxItem::BorderShape::Solid, parent)
        , customDataQueryId(customDataQueryId)
        , titleItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
        , labelCypher(new QGraphicsSimpleTextItem) //
        , queryCypherItem(new CustomGraphicsTextItem) //
        , queryCypherErrorMsgItem(new CustomGraphicsTextItem) //
        , labelParameters(new QGraphicsSimpleTextItem) //
        , queryParametersItem(new CustomGraphicsTextItem) //
        , queryParamsErrorMsgItem(new CustomGraphicsTextItem) //
        , labelQueryResult(new QGraphicsSimpleTextItem) //
        , textEdit(new CustomTextEdit(nullptr))
        , textEditProxyWidget(new QGraphicsProxyWidget) { //
}

void DataViewBox::setTitle(const QString &title) {
    titleItem->setPlainText(title);
    adjustContents();
}

void DataViewBox::setQuery(const QString &cypher, const QJsonObject &parameters) {
    queryCypherItem->setPlainText(cypher);
    queryParametersItem->setPlainText(
            parameters.isEmpty() ? "{}" : printJson(parameters, false));
    validateQueryCypher();
    validateQueryParameters();
    adjustContents();
}

void DataViewBox::setEditable(const bool editable) {
    titleItem->setEditable(editable);
    queryCypherItem->setEditable(editable);
    queryParametersItem->setEditable(editable);
}

void DataViewBox::setTextEditorIgnoreWheelEvent(const bool b) {
    textEditIgnoreWheelEvent = b;
}

int DataViewBox::getCustomDataQueryId() const {
    return customDataQueryId;
}

bool DataViewBox::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
    if (watched == textEditProxyWidget) {
        if (event->type() == QEvent::GraphicsSceneWheel) {
            if (textEditIgnoreWheelEvent)
                return true;

            if (!textEdit->isVerticalScrollBarVisible())
                return true;
        }
    }

    return BoardBoxItem::sceneEventFilter(watched, event);
}

QMenu *DataViewBox::createCaptionBarContextMenu() {
    auto *contextMenu = new QMenu;
    {
        auto *action = contextMenu->addAction(QIcon(":/icons/play_arrow_24"), "Run");
        connect(action, &QAction::triggered, this, [this]() {
            runQuery();
        });
    }
    contextMenu->addSeparator();
    {
        auto *action = contextMenu->addAction(QIcon(":/icons/close_box_black_24"), "Close");
        connect(action, &QAction::triggered, this, [this]() {
            const auto r = QMessageBox::question(getView(), " ", "Close the data query?");
            if (r == QMessageBox::Yes)
                emit closeByUser();
        });
    }

    return contextMenu;
}

void DataViewBox::setUpContents(QGraphicsItem *contentsContainer) {
    titleItem->setParentItem(contentsContainer);
    labelCypher->setParentItem(contentsContainer);
    queryCypherItem->setParentItem(contentsContainer);
    queryCypherErrorMsgItem->setParentItem(contentsContainer);
    labelParameters->setParentItem(contentsContainer);
    queryParametersItem->setParentItem(contentsContainer);
    queryParamsErrorMsgItem->setParentItem(contentsContainer);
    labelQueryResult->setParentItem(contentsContainer);
    textEditProxyWidget->setParentItem(contentsContainer);

    // get view's font
    QFont fontOfView;
    if (QGraphicsView *view = getView(); view != nullptr)
        fontOfView = view->font();

    // titleItem
    {
        QFont font = fontOfView;
        font.setPixelSize(24);
        font.setBold(true);

        titleItem->setFont(font);
        titleItem->setDefaultTextColor(Qt::black);
    }

    // labelCypher, labelParameters, & labelQueryResult
    {
        QFont font = fontOfView;
        font.setPixelSize(13);
        font.setBold(true);

        const QColor textColor(127, 127, 127);

        labelCypher->setText("Cypher:");
        labelCypher->setFont(font);
        labelCypher->setBrush(textColor);

        labelParameters->setText("Parameters:");
        labelParameters->setFont(font);
        labelParameters->setBrush(textColor);

        labelQueryResult->setText("Result:");
        labelQueryResult->setFont(font);
        labelQueryResult->setBrush(textColor);
    }

    // queryCypherItem & queryParametersItem
    {
        const QColor textColor(Qt::black);

        QFont font = fontOfView;
        font.setFamily(qApp->property("monospaceFontFamily").toString());
        font.setPixelSize(16);

        queryCypherItem->setFont(font);
        queryCypherItem->setDefaultTextColor(textColor);

        queryParametersItem->setFont(font);
        queryParametersItem->setDefaultTextColor(textColor);
    }

    // queryCypherErrorMsg & queryParamsErrorMsg
    {
        const QColor textColor(Qt::red);

        QFont font = fontOfView;
        font.setPixelSize(13);

        queryCypherErrorMsgItem->setFont(font);
        queryCypherErrorMsgItem->setDefaultTextColor(textColor);

        queryParamsErrorMsgItem->setFont(font);
        queryParamsErrorMsgItem->setDefaultTextColor(textColor);
    }

    // textEditProxyWidget & textEdit
    textEdit->setVisible(false);
    textEditProxyWidget->setWidget(textEdit);

    textEdit->enableSetEveryWheelEventAccepted(true);
    textEdit->setReadOnly(true);
    textEdit->setFrameShape(QFrame::NoFrame);
    textEdit->setMinimumHeight(10);
    textEdit->setContextMenuPolicy(Qt::NoContextMenu);

    constexpr int textEditFontPixelSize = 16;
    const QString fontFamily = qApp->property("monospaceFontFamily").toString();
    textEdit->setStyleSheet(
            "QTextEdit {"
            "  font-family: \"" + fontFamily + "\";"
            "  font-size: " + QString::number(textEditFontPixelSize) + "px;"
            "}"
            "QScrollBar:vertical {"
            "  width: 12px;"
            "}"
    );

    //
    const QString dataQueryIdStr
            = (customDataQueryId >= 0) ? QString("Data Query %1").arg(customDataQueryId) : "";
    setCaptionBarRightText(dataQueryIdStr);

    // ==== install event filter ====

    textEditProxyWidget->installSceneEventFilter(this);

    // ==== connections ====

    // titleItem
    connect(titleItem, &CustomGraphicsTextItem::textEdited, this, [this](bool heightChanged) {
        emit titleUpdated(titleItem->toPlainText());
        if (heightChanged)
            adjustContents();
    });

    connect(titleItem, &CustomGraphicsTextItem::clicked, this, [this]() {
        emit clicked();
    });

    // queryCypherItem
    connect(queryCypherItem, &CustomGraphicsTextItem::textEdited, this, [this](bool heightChanged) {
        // validate
        const auto [validateOk1, errorMsgChanged1] = validateQueryCypher();
        const auto [validateOk2, errorMsgChanged2] = validateQueryParameters();

        const bool validateOk = validateOk1 && validateOk2;
        const bool errorMsgChanged = errorMsgChanged1 || errorMsgChanged2;

        //
        if (validateOk) {
            const QJsonObject parameters = parseAsJsonObject(queryParametersItem->toPlainText());
            emit queryUpdated(queryCypherItem->toPlainText(), parameters);
        }

        //
        if (heightChanged || errorMsgChanged)
            adjustContents();
    });

    connect(queryCypherItem, &CustomGraphicsTextItem::clicked, this, [this]() {
        emit clicked();
    });

    // queryParametersItem
    connect(queryParametersItem, &CustomGraphicsTextItem::textEdited, this, [this](bool heightChanged) {
        // validate
        const auto [validateOk1, errorMsgChanged1] = validateQueryCypher();
        const auto [validateOk2, errorMsgChanged2] = validateQueryParameters();

        const bool validateOk = validateOk1 && validateOk2;
        const bool errorMsgChanged = errorMsgChanged1 || errorMsgChanged2;

        //
        if (validateOk) {
            const QJsonObject parameters = parseAsJsonObject(queryParametersItem->toPlainText());
            emit queryUpdated(queryCypherItem->toPlainText(), parameters);
        }

        //
        if (heightChanged || errorMsgChanged)
            adjustContents();
    });

    connect(queryParametersItem, &CustomGraphicsTextItem::clicked, this, [this]() {
        emit clicked();
    });
}

void DataViewBox::adjustContents() {
    const QRectF contentsRect = getContentsRect();

    // get view's font
    QFont fontOfView;
    if (QGraphicsView *view = getView(); view != nullptr)
        fontOfView = view->font();

    // title
    double yBottom = contentsRect.top();
    {
        constexpr int padding = 3;
        const double minHeight = QFontMetrics(titleItem->font()).height();

        //
        titleItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        titleItem->setPos(contentsRect.topLeft() + QPointF(padding, padding));

        yBottom += std::max(titleItem->boundingRect().height(), minHeight)
                   + padding * 2;
    }

    // labelCypher
    {
        constexpr int xPadding = 3;
        labelCypher->setPos(contentsRect.left() + xPadding, yBottom);

        yBottom += labelCypher->boundingRect().height();
    }

    // queryCypherItem
    {
        constexpr int padding = 3;
        const double minHeight = QFontMetrics(queryCypherItem->font()).height();

        queryCypherItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        queryCypherItem->setPos(contentsRect.left() + padding, yBottom + padding);

        yBottom += std::max(queryCypherItem->boundingRect().height(), minHeight)
                   + padding * 2;
    }

    // queryCypherErrorMsg
    {
        if (queryCypherErrorMsgItem->toPlainText().trimmed().isEmpty()) {
            queryCypherErrorMsgItem->setVisible(false);
        }
        else {
            constexpr int xPadding = 3;
            constexpr int bottomPadding = 3;

            queryCypherErrorMsgItem->setTextWidth(
                    std::max(contentsRect.width() - xPadding * 2, 0.0));
            queryCypherErrorMsgItem->setPos(
                    contentsRect.left() + xPadding, yBottom);
            queryCypherErrorMsgItem->setVisible(true);

            yBottom += queryCypherErrorMsgItem->boundingRect().height()
                       + bottomPadding;
        }
    }

    // labelParameters
    {
        constexpr int xPadding = 3;
        labelParameters->setPos(contentsRect.left() + xPadding, yBottom);

        yBottom += labelParameters->boundingRect().height();
    }

    // queryParametersItem
    {
        constexpr int padding = 3;

        queryParametersItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        queryParametersItem->setPos(
                contentsRect.left() + padding, yBottom + padding);

        yBottom += queryParametersItem->boundingRect().height()
                   + padding * 2;
    }

    // queryParamsErrorMsg
    {
        if (queryParamsErrorMsgItem->toPlainText().trimmed().isEmpty()) {
            queryParamsErrorMsgItem->setVisible(false);
        }
        else {
            constexpr int xPadding = 3;
            constexpr int bottomPadding = 3;

            queryParamsErrorMsgItem->setTextWidth(
                    std::max(contentsRect.width() - xPadding * 2, 0.0));
            queryParamsErrorMsgItem->setPos(
                    contentsRect.left() + xPadding, yBottom);
            queryParamsErrorMsgItem->setVisible(true);

            yBottom += queryParamsErrorMsgItem->boundingRect().height()
                       + bottomPadding;
        }
    }

    // labelQueryResult
    {
        constexpr int xPadding = 3;
        labelQueryResult->setPos(contentsRect.left() + xPadding, yBottom);

        yBottom += labelQueryResult->boundingRect().height();
    }

    // textEdit
    {
        constexpr int leftPadding = 3;

        const double textEditHeight = contentsRect.bottom() - yBottom;
        if (textEditHeight < 0.1) {
            textEditProxyWidget->setVisible(false);
        }
        else {
            textEditProxyWidget->resize(contentsRect.width() - leftPadding, textEditHeight);
            textEditProxyWidget->setVisible(true);
        }

        textEditProxyWidget->setPos(contentsRect.left() + leftPadding, yBottom);
    }
}

std::pair<bool, bool> DataViewBox::validateQueryCypher() {
    const QString cypher = queryCypherItem->toPlainText();
    QString errorMsg;
    const bool validateOk = CustomDataQuery::validateCypher(cypher, &errorMsg);

    const QString oldErrorMsg = queryCypherErrorMsgItem->toPlainText();
    queryCypherErrorMsgItem->setPlainText(errorMsg);

    return {validateOk, errorMsg != oldErrorMsg};
}

std::pair<bool, bool> DataViewBox::validateQueryParameters() {
    bool validateOk;
    {
        QString errorMsg;
        parseAsJsonObject(queryParametersItem->toPlainText(), &errorMsg);
        validateOk = errorMsg.isEmpty();
    }

    const QString oldErrorMsg = queryParamsErrorMsgItem->toPlainText();
    const QString displayErrorMsg = validateOk ? "" : "must be a JSON object";
    queryParamsErrorMsgItem->setPlainText(displayErrorMsg);

    return {validateOk, displayErrorMsg != oldErrorMsg};
}

void DataViewBox::runQuery() {
    const bool validateOk = validateQueryCypher().first && validateQueryParameters().first;
    if (!validateOk)
        return;

    //
    textEdit->setPlainText("performing query...");

    // add parameter "cardIdsOfBoard"
    constexpr char keyCardIdsOfBoard[] = "cardIdsOfBoard";

    QJsonObject parameters = parseAsJsonObject(queryParametersItem->toPlainText());
    if (!parameters.contains(keyCardIdsOfBoard)) {
        QSet<int> cardIds;
        emit getCardIdsOfBoard(&cardIds);

        parameters.insert(keyCardIdsOfBoard, toJsonArray(cardIds));
    }

    //
    Services::instance()->getAppDataReadonly()->performCustomCypherQuery(
            queryCypherItem->toPlainText(),
            parameters,
            // callback
            [this](bool ok, const QVector<QJsonObject> &rows) {
                if (!ok) {
                    const QString errorMsg = rows.at(0).value("errorMsg").toString();
                    textEdit->setPlainText(QString("Query failed:\n%1").arg(errorMsg));
                }
                else {
                    QStringList lines;
                    for (const QJsonObject &row: rows)
                        lines << printJson(row);
                    textEdit->setPlainText(lines.join("\n\n"));
                }
            },
            this
    );
}
