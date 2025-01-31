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
#include "widgets/widgets_constants.h"

DataViewBox::DataViewBox(const int customDataQueryId, QGraphicsItem *parent)
        : BoardBoxItem(BoardBoxItem::CreationParameters {}, parent)
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

DataViewBox::~DataViewBox() {
    // Handle the TextEdit embeded in `textEditProxyWidget` exclusively. Without this, the program
    // crashes for unknown reason.
    auto *textEdit = textEditProxyWidget->widget();
    if (textEdit != nullptr) {
        textEditProxyWidget->setWidget(nullptr);
        textEdit->deleteLater();
        // Using `delete textEdit;` also makes the program crash. It seems that `textEdit` is still
        // accessed later.
    }
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
    using TextInteractionState = CustomGraphicsTextItem::TextInteractionState;
    const TextInteractionState state = editable
             ? TextInteractionState::Editable : TextInteractionState::Selectable;

    titleItem->setTextInteractionState(state);
    queryCypherItem->setTextInteractionState(state);
    queryParametersItem->setTextInteractionState(state);
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
        auto *action = contextMenu->addAction("Run");
        contextMenuActionToIcon.insert(action, Icon::PlayArrow);
        connect(action, &QAction::triggered, this, [this]() {
            runQuery([](bool /*ok*/, const QVector<QJsonObject> &/*result*/) {});
        });
    }
    {
        auto *action = contextMenu->addAction("Run and Export");
        contextMenuActionToIcon.insert(action, Icon::PlayArrow);
        connect(action, &QAction::triggered, this, [this]() {
            runQuery([this](bool ok, const QVector<QJsonObject> &result) {
                if (ok)
                    this->exportQueryResult(result);
            });
        });
    }
    contextMenu->addSeparator();
    {
        auto *action = contextMenu->addAction("Close");
        contextMenuActionToIcon.insert(action, Icon::CloseBox);
        connect(action, &QAction::triggered, this, [this]() {
            const auto r = QMessageBox::question(getView(), " ", "Close the data query?");
            if (r == QMessageBox::Yes)
                emit closeByUser();
        });
    }

    return contextMenu;
}

void DataViewBox::adjustCaptionBarContextMenuBeforePopup(QMenu */*contextMenu*/) {
    // set action icons
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = contextMenuActionToIcon.constBegin();
            it != contextMenuActionToIcon.constEnd(); ++it) {
        it.key()->setIcon(Icons::getIcon(it.value(), theme));
    }
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

    //
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    const QColor titleTextColor = getTitleItemDefaultTextColor(isDarkTheme);

    // titleItem
    {
        QFont font = fontOfView;
        font.setPixelSize(20);
        font.setBold(true);

        titleItem->setFont(font);
        titleItem->setDefaultTextColor(titleTextColor);
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
        QFont font = fontOfView;
        font.setFamily(qApp->property("monospaceFontFamily").toString());
        font.setPixelSize(16);

        queryCypherItem->setFont(font);
        queryCypherItem->setDefaultTextColor(titleTextColor);

        queryParametersItem->setFont(font);
        queryParametersItem->setDefaultTextColor(titleTextColor);
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
        emit leftButtonPressedOrClicked();
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
        emit leftButtonPressedOrClicked();
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
        emit leftButtonPressedOrClicked();
    });

    //
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {
        const QColor textColor = getTitleItemDefaultTextColor(isDarkTheme);
        titleItem->setDefaultTextColor(textColor);
        queryCypherItem->setDefaultTextColor(textColor);
        queryParametersItem->setDefaultTextColor(textColor);
    });
}

void DataViewBox::adjustContents() {
    const QRectF contentsRect = getContentsRect();

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

void DataViewBox::onMouseLeftPressed(
        const bool /*isOnCaptionBar*/, const Qt::KeyboardModifiers modifiers) {
    if (modifiers == Qt::NoModifier)
        emit leftButtonPressedOrClicked();
}

void DataViewBox::onMouseLeftClicked(
        const bool /*isOnCaptionBar*/, const Qt::KeyboardModifiers /*modifiers*/) {
    // do nothing
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

void DataViewBox::runQuery(
        std::function<void (bool ok, const QVector<QJsonObject> &result)> callback) {
    const bool validateOk = validateQueryCypher().first && validateQueryParameters().first;
    if (!validateOk) {
        callback(false, {});
        return;
    }

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
            [this, callback](bool ok, const QVector<QJsonObject> &rows) {
                if (!ok) {
                    const QString errorMsg = rows.at(0).value("errorMsg").toString();
                    textEdit->setPlainText(QString("Query failed:\n%1").arg(errorMsg));
                    //
                    callback(false, {});
                }
                else {
                    QStringList lines;
                    for (const QJsonObject &row: rows)
                        lines << printJson(row);
                    textEdit->setPlainText(lines.join("\n\n"));
                    callback(true, rows);
                }
            },
            this
    );
}

void DataViewBox::exportQueryResult(const QVector<QJsonObject> &queryResult) const {

}

QColor DataViewBox::getTitleItemDefaultTextColor(const bool isDarkTheme) {
    return isDarkTheme ? QColor(darkThemeStandardTextColor) : QColor(Qt::black);
}
