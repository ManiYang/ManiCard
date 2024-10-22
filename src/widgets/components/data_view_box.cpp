#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMessageBox>
#include "data_view_box.h"
#include "models/data_query.h"

DataViewBox::DataViewBox(const int dataQueryId, QGraphicsItem *parent)
        : BoardBoxItem(parent)
        , dataQueryId(dataQueryId)
        , titleItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
        , queryCypherItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
        , queryCypherErrorMsgItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
        , textEdit(new CustomTextEdit(nullptr))
        , textEditProxyWidget(new QGraphicsProxyWidget) { // parent is set in setUpContents()
}

void DataViewBox::setTitle(const QString &title) {
    titleItem->setPlainText(title);
    adjustContents();
}

void DataViewBox::setQuery(const QString &cypher, const QJsonObject &parameters) {
    queryCypherItem->setPlainText(cypher);
    validateCypher();
    adjustContents();
}

void DataViewBox::setEditable(const bool editable) {
    titleItem->setEditable(editable);
    queryCypherItem->setEditable(editable);
}

void DataViewBox::setTextEditorIgnoreWheelEvent(const bool b) {
    textEditIgnoreWheelEvent = b;
}

int DataViewBox::getDataQueryId() const {
    return dataQueryId;
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
    queryCypherItem->setParentItem(contentsContainer);
    queryCypherErrorMsgItem->setParentItem(contentsContainer);
    textEditProxyWidget->setParentItem(contentsContainer);

    //
    textEdit->setVisible(false);
    textEditProxyWidget->setWidget(textEdit);

    //
    textEdit->enableSetEveryWheelEventAccepted(true);
    textEdit->setReadOnly(true);
    textEdit->setFrameShape(QFrame::NoFrame);
    textEdit->setMinimumHeight(10);
    textEdit->setContextMenuPolicy(Qt::NoContextMenu);

    constexpr int textEditFontPixelSize = 16;
    textEdit->setStyleSheet(
            "QTextEdit {"
            "  font-size: " + QString::number(textEditFontPixelSize) + "px;"
            "}"
            "QScrollBar:vertical {"
            "  width: 12px;"
            "}"
    );

    //
    const QString dataQueryIdStr = (dataQueryId >= 0) ? QString("Data Query %1").arg(dataQueryId) : "";
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
        const auto [validateOk, errorMsgChanged] = validateCypher();

        //
        if (validateOk) {
            const QJsonObject parameters {}; // temp
            emit queryUpdated(queryCypherItem->toPlainText(), parameters);
        }

        //
        if (heightChanged || errorMsgChanged)
            adjustContents();
    });

    connect(queryCypherItem, &CustomGraphicsTextItem::clicked, this, [this]() {
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
    double yTitleBottom;
    {
        constexpr int padding = 3;
        constexpr int fontPixelSize = 24;
        const QColor textColor(Qt::black);

        //
        QFont font = fontOfView;
        font.setPixelSize(fontPixelSize);
        font.setBold(true);

        const double minHeight = QFontMetrics(font).height();

        //
        titleItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        titleItem->setFont(font);
        titleItem->setDefaultTextColor(textColor);
        titleItem->setPos(contentsRect.topLeft() + QPointF(padding, padding));

        yTitleBottom
                = contentsRect.top()
                  + std::max(titleItem->boundingRect().height(), minHeight)
                  + padding * 2;
    }

    // queryCypher
    double yQueryCypherBottom;
    {
        constexpr int padding = 3;
        constexpr int fontPixelSize = 16;
        const QColor textColor(Qt::black);

        //
        QFont font = fontOfView;
        font.setPixelSize(fontPixelSize);

        const double minHeight = QFontMetrics(font).height();

        //
        queryCypherItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        queryCypherItem->setFont(font);
        queryCypherItem->setDefaultTextColor(textColor);
        queryCypherItem->setPos(contentsRect.left() + padding, yTitleBottom + padding);

        yQueryCypherBottom
                = yTitleBottom
                  + std::max(queryCypherItem->boundingRect().height(), minHeight)
                  + padding * 2;
    }

    // queryCypherErrorMsg
    double yQueryCypherErrorMsgBottom;
    {
        if (queryCypherErrorMsgItem->toPlainText().trimmed().isEmpty()) {
            queryCypherErrorMsgItem->setVisible(false);
            yQueryCypherErrorMsgBottom = yQueryCypherBottom;
        }
        else {
            constexpr int padding = 3;
            constexpr int fontPixelSize = 13;
            const QColor textColor(Qt::red);

            QFont font = fontOfView;
            font.setPixelSize(fontPixelSize);

            queryCypherErrorMsgItem->setTextWidth(
                    std::max(contentsRect.width() - padding * 2, 0.0));
            queryCypherErrorMsgItem->setFont(font);
            queryCypherErrorMsgItem->setDefaultTextColor(textColor);
            queryCypherErrorMsgItem->setPos(
                    contentsRect.left() + padding, yQueryCypherBottom + padding);
            queryCypherErrorMsgItem->setVisible(true);

            yQueryCypherErrorMsgBottom
                    = yQueryCypherBottom
                      + queryCypherErrorMsgItem->boundingRect().height()
                      + padding * 2;
        }
    }

    // textEdit
    {
        constexpr int leftPadding = 3;

        const double textEditHeight = contentsRect.bottom() - yQueryCypherErrorMsgBottom;
        if (textEditHeight < 0.1) {
            textEditProxyWidget->setVisible(false);
        }
        else {
            textEditProxyWidget->resize(contentsRect.width() - leftPadding, textEditHeight);
            textEditProxyWidget->setVisible(true);
        }

        textEditProxyWidget->setPos(contentsRect.left() + leftPadding, yQueryCypherErrorMsgBottom);
    }
}

std::pair<bool, bool> DataViewBox::validateCypher() {
    const QString cypher = queryCypherItem->toPlainText();
    QString errorMsg;
    const bool validateOk = DataQuery::validateCypher(cypher, &errorMsg);

    const QString oldErrorMsg = queryCypherErrorMsgItem->toPlainText();
    queryCypherErrorMsgItem->setPlainText(errorMsg);

    return {validateOk, errorMsg != oldErrorMsg};
}
