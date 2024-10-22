#include <QGraphicsScene>
#include <QMessageBox>
#include "node_rect.h"
#include "utilities/margins_util.h"
#include "widgets/components/custom_graphics_text_item.h"
#include "widgets/components/custom_text_edit.h"

NodeRect::NodeRect(const int cardId, QGraphicsItem *parent)
    : BoardBoxItem(parent)
    , cardId(cardId)
    , titleItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
    , textEdit(new CustomTextEdit(nullptr))
    , textEditProxyWidget(new QGraphicsProxyWidget) // parent is set in setUpContents()
    , textEditFocusIndicator(new QGraphicsRectItem(this)){
}

void NodeRect::setNodeLabels(const QStringList &labels) {
    nodeLabels = labels;

    constexpr bool bold = true;
    setCaptionBarLeftText(getNodeLabelsString(nodeLabels), bold);
}

void NodeRect::setTitle(const QString &title) {
    titleItem->setPlainText(title);
    adjustContents();
}

void NodeRect::setText(const QString &text) {
    const double textEditLineHeightPercent {120};

    textEdit->setPlainText(text);

    // set line height of whole document
    auto cursor = textEdit->textCursor();
    auto blockFormat = cursor.blockFormat();
    blockFormat.setLineHeight(
            textEditLineHeightPercent, QTextBlockFormat::ProportionalHeight);
    cursor.select(QTextCursor::Document);
    cursor.setBlockFormat(blockFormat);

    //
    adjustContents();
}

void NodeRect::setEditable(const bool editable) {
    titleItem->setEditable(editable);
    textEdit->setReadOnly(!editable);
}

void NodeRect::setTextEditorIgnoreWheelEvent(const bool b) {
    textEditIgnoreWheelEvent = b;
}

int NodeRect::getCardId() const {
    return cardId;
}

QSet<QString> NodeRect::getNodeLabels() const {
    return QSet<QString>(nodeLabels.constBegin(), nodeLabels.constEnd());
}

QString NodeRect::getTitle() const {
    return titleItem->toPlainText();
}

QString NodeRect::getText() const {
    return textEdit->toPlainText();
}

bool NodeRect::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
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

QMenu *NodeRect::createCaptionBarContextMenu() {
    auto *contextMenu = new QMenu;
    {
        auto *action = contextMenu->addAction(
                QIcon(":/icons/label_black_24"), "Set Labels...");
        connect(action, &QAction::triggered, this, [this]() {
            emit userToSetLabels();
        });
    }
    {
        auto *action = contextMenu->addAction(
                QIcon(":/icons/arrow_right_black_24"), "Create Relationship...");
        connect(action, &QAction::triggered, this, [this]() {
            emit userToCreateRelationship();
        });
    }
    contextMenu->addSeparator();
    {
        auto *action = contextMenu->addAction(QIcon(":/icons/close_box_black_24"), "Close");
        connect(action, &QAction::triggered, this, [this]() {
            const auto r = QMessageBox::question(getView(), " ", "Close the card?");
            if (r == QMessageBox::Yes)
                emit closeByUser();
        });
    }

    return contextMenu;
}

void NodeRect::setUpContents(QGraphicsItem *contentsContainer) {
    titleItem->setParentItem(contentsContainer);
    textEditProxyWidget->setParentItem(contentsContainer);

    //
    textEditProxyWidget->setWidget(textEdit);

    //
    textEdit->enableSetEveryWheelEventAccepted(true);
    textEdit->setVisible(false);
    textEdit->setReadOnly(true);
    textEdit->setReplaceTabBySpaces(4);
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
    textEditFocusIndicator->setBrush(Qt::NoBrush);
    textEditFocusIndicator->setPen(
            QPen(QBrush(QColor(195, 225, 255)), textEditFocusIndicatorLineWidth));
    textEditFocusIndicator->setVisible(false);

    //
    const QString cardIdStr = (cardId >= 0) ? QString("Card %1").arg(cardId) : "";
    setCaptionBarRightText(cardIdStr);

    // ==== install event filter ====

    textEditProxyWidget->installSceneEventFilter(this);

    // ==== connections ====

    // titleItem
    connect(titleItem, &CustomGraphicsTextItem::textEdited, this, [this](bool heightChanged) {
        emit titleTextUpdated(titleItem->toPlainText(), std::nullopt);
        if (heightChanged)
            adjustContents();
    });

    connect(titleItem, &CustomGraphicsTextItem::clicked, this, [this]() {
        emit clicked();
    });

    // textEdit
    connect(textEdit, &CustomTextEdit::textEdited, this, [this]() {
        emit titleTextUpdated(std::nullopt, textEdit->toPlainText());
    });

    connect(textEdit, &CustomTextEdit::clicked, this, [this]() {
        emit clicked();
    });

    connect(textEdit, &CustomTextEdit::focusedIn, this, [this]() {
        textEditFocusIndicator->setVisible(true);
    });

    connect(textEdit, &CustomTextEdit::focusedOut, this, [this]() {
        textEditFocusIndicator->setVisible(false);
    });
}

void NodeRect::adjustContents() {
    const QRectF contentsRect = getContentsRect();

    // get view's font
    QFont fontOfView;
    if (QGraphicsView *view = getView(); view != nullptr)
        fontOfView = view->font();

    // title
    double yTitleBottom = 0;
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

    // text
    double textEditHeight;
    {
        constexpr int leftPadding = 3;

        textEditHeight = contentsRect.bottom() - yTitleBottom;
        if (textEditHeight < 0.1) {
            textEditProxyWidget->setVisible(false);
        }
        else {
            textEditProxyWidget->resize(contentsRect.width() - leftPadding, textEditHeight);
            textEditProxyWidget->setVisible(true);
        }

        textEditProxyWidget->setPos(contentsRect.left() + leftPadding, yTitleBottom);
    }

    // textEditFocusIndicator
    textEditFocusIndicator->setRect(
            QRectF(contentsRect.left(), yTitleBottom - 2.0,
                   contentsRect.width(), textEditHeight + 2.0)
                .marginsRemoved(uniformMarginsF(textEditFocusIndicatorLineWidth / 2.0))
                );
}

QGraphicsView *NodeRect::getView() const {
    if (scene() == nullptr)
        return nullptr;

    if (const auto views = scene()->views(); !views.isEmpty())
        return views.at(0);
    else
        return nullptr;
}

QString NodeRect::getNodeLabelsString(const QStringList &labels) {
    QStringList labels2;
    for (const QString &label: labels)
        labels2 << (":" + label);
    return labels2.join(" ");
}
