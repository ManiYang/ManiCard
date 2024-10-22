#include <QGraphicsScene>
#include <QGraphicsView>
#include "data_view_box.h"

DataViewBox::DataViewBox(const int dataQueryId, QGraphicsItem *parent)
        : BoardBoxItem(parent)
        , dataQueryId(dataQueryId)
        , titleItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
        , queryItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
        , textEdit(new CustomTextEdit(nullptr))
        , textEditProxyWidget(new QGraphicsProxyWidget) { // parent is set in setUpContents()
}

void DataViewBox::setTitle(const QString &title) {
    titleItem->setPlainText(title);
    adjustContents();
}

void DataViewBox::setQuery(const QString &query) {
    queryItem->setPlainText(query);
    adjustContents();
}

void DataViewBox::setEditable(const bool editable) {
    titleItem->setEditable(editable);
    queryItem->setEditable(editable);
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
    return nullptr;
}

void DataViewBox::setUpContents(QGraphicsItem *contentsContainer) {
    titleItem->setParentItem(contentsContainer);
    queryItem->setParentItem(contentsContainer);
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

    // queryItem
    connect(queryItem, &CustomGraphicsTextItem::textEdited, this, [this](bool heightChanged) {
        emit queryUpdated(queryItem->toPlainText());
        if (heightChanged)
            adjustContents();
    });

    connect(queryItem, &CustomGraphicsTextItem::clicked, this, [this]() {
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

    // query
    double yQueryBottom;
    {
        constexpr int padding = 3;
        constexpr int fontPixelSize = 16;
        const QColor textColor(Qt::black);

        //
        QFont font = fontOfView;
        font.setPixelSize(fontPixelSize);

        const double minHeight = QFontMetrics(font).height();

        //
        queryItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        queryItem->setFont(font);
        queryItem->setDefaultTextColor(textColor);
        queryItem->setPos(contentsRect.left() + padding, yTitleBottom + padding);

        yQueryBottom
                = yTitleBottom
                  + std::max(queryItem->boundingRect().height(), minHeight)
                  + padding * 2;
    }

    // textEdit
    {
        constexpr int leftPadding = 3;

        const double textEditHeight = contentsRect.bottom() - yQueryBottom;
        if (textEditHeight < 0.1) {
            textEditProxyWidget->setVisible(false);
        }
        else {
            textEditProxyWidget->resize(contentsRect.width() - leftPadding, textEditHeight);
            textEditProxyWidget->setVisible(true);
        }

        textEditProxyWidget->setPos(contentsRect.left() + leftPadding, yQueryBottom);
    }
}
