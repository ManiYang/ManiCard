#include <algorithm>
#include <QBrush>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QPainter>
#include <QPen>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsView>
#include <QMessageBox>
#include <QTextBlockFormat>
#include <QTextCursor>
#include "models/node_labels.h"
#include "node_rect.h"
#include "utilities/margins_util.h"
#include "widgets/components/custom_text_edit.h"
#include "widgets/components/graphics_item_move_resize.h"
#include "widgets/components/custom_graphics_text_item.h"

NodeRect::NodeRect(const int cardId_, QGraphicsItem *parent)
        : QGraphicsObject(parent)
        , cardId(cardId_)
        , captionBarItem(new QGraphicsRectItem(this))
        , nodeLabelItem(new QGraphicsSimpleTextItem(this))
        , cardIdItem(new QGraphicsSimpleTextItem(this))
        , contentsRectItem(new QGraphicsRectItem(this))
        , titleItem(new CustomGraphicsTextItem(contentsRectItem))
        , textEdit(new CustomTextEdit(true, nullptr))
        , textEditProxyWidget(new QGraphicsProxyWidget(contentsRectItem))
        , textEditFocusIndicator(new QGraphicsRectItem(this))
        , moveResizeHelper(new GraphicsItemMoveResize(this))
        , contextMenu(new QMenu) {
    textEdit->setVisible(false);
    textEdit->setReadOnly(true);
    textEdit->setReplaceTabBySpaces(4);
    textEditProxyWidget->setWidget(textEdit);

    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    setAcceptHoverEvents(true);

    setUpContextMenu();
    setUpConnections();
    adjustChildItems();
}

NodeRect::~NodeRect() {
    contextMenu->deleteLater();
}

void NodeRect::initialize() {
    Q_ASSERT(scene() != nullptr);

    moveResizeHelper->setMoveHandle(captionBarItem);

    constexpr double resizeAreaMaxWidth = 6.0;
    moveResizeHelper->setResizeHandle(this, resizeAreaMaxWidth, minSizeForResizing);

    //
    installEventFilterOnChildItems();
}

void NodeRect::setRect(const QRectF rect_) {
    prepareGeometryChange();
    enclosingRect = rect_;
    redraw();
}

void NodeRect::setColor(const QColor color_) {
    color = color_;
    redraw();
}

void NodeRect::setMarginWidth(const double width) {
    marginWidth = width;
    redraw();
}

void NodeRect::setBorderWidth(const double width) {
    borderWidth = width;
    redraw();
}

void NodeRect::setNodeLabels(const QStringList &labels) {
    nodeLabels = labels;
    adjustChildItems();
}

void NodeRect::setNodeLabels(const QVector<QString> &labels) {
    nodeLabels = QStringList(labels.constBegin(), labels.constEnd());
    adjustChildItems();
}

void NodeRect::setTitle(const QString &title) {
    titleItem->setPlainText(title);
    adjustChildItems();
}

void NodeRect::setText(const QString &text) {
    textEdit->setPlainText(text);
    {
        // set line height of whole document
        auto cursor = textEdit->textCursor();
        auto blockFormat = cursor.blockFormat();
        blockFormat.setLineHeight(
                textEditLineHeightPercent, QTextBlockFormat::ProportionalHeight);
        cursor.select(QTextCursor::Document);
        cursor.setBlockFormat(blockFormat);
    }

    adjustChildItems();
}

void NodeRect::setEditable(const bool editable) {
    isEditable = editable;

    titleItem->setEditable(editable);
    textEdit->setReadOnly(!isEditable);
}

void NodeRect::setHighlighted(const bool highlighted) {
    if (isHighlighted != highlighted) {
        isHighlighted = highlighted;
        update();
    }
}

QRectF NodeRect::getRect() const {
    return enclosingRect;
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

bool NodeRect::getIsHighlighted() const {
    return isHighlighted;
}

QRectF NodeRect::boundingRect() const {
    return enclosingRect.marginsAdded(uniformMarginsF(marginWidth));
}

void NodeRect::paint(
        QPainter *painter, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    painter->save();

    // background rect
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    const double radius = borderWidth;
    painter->drawRoundedRect(enclosingRect, radius, radius);

    // highlight box
    if (isHighlighted) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(getHighlightBoxColor(color), highlightBoxWidth));
        const double radius = borderWidth;
        painter->drawRoundedRect(
                enclosingRect
                    .marginsAdded(uniformMarginsF(marginWidth))
                    .marginsRemoved(uniformMarginsF(highlightBoxWidth / 2.0)),
                radius, radius);
    }

    //
    painter->restore();
}

void NodeRect::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    event->accept();
}

bool NodeRect::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
    if (watched == captionBarItem || watched == nodeLabelItem || watched == cardIdItem) {
        if (event->type() == QEvent::GraphicsSceneContextMenu) {
            auto *e = dynamic_cast<QGraphicsSceneContextMenuEvent *>(event);
            const auto pos = e->screenPos();
            contextMenu->popup(pos);
            return true;
        }
        else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            emit clicked();
        }
    }

    return false;
}

void NodeRect::mousePressEvent(QGraphicsSceneMouseEvent */*event*/) {
    // do nothing

    // This method is reimplemented so that
    // 1. the mouse-press event is accepted and `this` becomes the mouse grabber, and
    // 2. the later mouse-release event will be sent to `this` and will not "penetrate"
    //    through `this` and reach the QGraphicsScene.
}

void NodeRect::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsObject::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
        emit clicked();
}

void NodeRect::installEventFilterOnChildItems() {
    captionBarItem->installSceneEventFilter(this);
    nodeLabelItem->installSceneEventFilter(this);
    cardIdItem->installSceneEventFilter(this);
}

void NodeRect::setUpContextMenu() {
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
}

void NodeRect::setUpConnections() {
    connect(titleItem, &CustomGraphicsTextItem::textEdited, this, [this](bool heightChanged) {
        emit titleTextUpdated(titleItem->toPlainText(), std::nullopt);
        if (heightChanged)
            adjustChildItems();
    });

    connect(titleItem, &CustomGraphicsTextItem::clicked, this, [this]() {
        emit clicked();
    });

    //
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

    // ==== moveResizeHelper ====

    connect(moveResizeHelper, &GraphicsItemMoveResize::getTargetItemPosition,
            this, [this](QPointF *pos) {
        *pos = enclosingRect.topLeft();
    }, Qt::DirectConnection);

    connect(moveResizeHelper, &GraphicsItemMoveResize::setTargetItemPosition,
            this, [this](const QPointF &pos) {
        prepareGeometryChange();
//        qDebug() << "-->" << pos;
        enclosingRect.moveTopLeft(pos);
        redraw();

        scene()->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
        // this is to deal with the QGraphicsView problem
        // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug

        emit movedOrResized();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::movingEnded, this, [this]() {
        emit finishedMovingOrResizing();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::getTargetItemRect,
            this, [this](QRectF *rect) {
        *rect = enclosingRect;
    }, Qt::DirectConnection);

    connect(moveResizeHelper, &GraphicsItemMoveResize::setTargetItemRect,
            this, [this](const QRectF &rect) {
        prepareGeometryChange();
        enclosingRect = rect;
        redraw();

        emit movedOrResized();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::resizingEnded, this, [this]() {
        emit finishedMovingOrResizing();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::setCursorShape,
            this, [this](const std::optional<Qt::CursorShape> cursorShape) {
        if (cursorShape.has_value())
            setCursor(cursorShape.value());
        else
            unsetCursor();
    });
}

void NodeRect::adjustChildItems() {
//    qDebug().noquote() << "adjustChildItems()";

    // get view's font
    QFont fontOfView;
    if (scene() != nullptr) {
        if (const auto views = scene()->views(); !views.isEmpty()) {
            auto *view = views.at(0);
            fontOfView = view->font();
        }
    }

    //
    const auto borderInnerRect
            = enclosingRect.marginsRemoved(uniformMarginsF(borderWidth));

    // caption bar
    double captionHeight = 0;
    {
        constexpr int padding = 2;
        constexpr int fontPointSize = 10;
        const QString fontFamily = "Arial";
        const QColor textColor(Qt::white);

        //
        QFont normalFont = fontOfView;
        normalFont.setFamily(fontFamily);
        normalFont.setPointSize(fontPointSize);

        QFont boldFont = normalFont;
        boldFont.setBold(true);

        const int fontHeight = QFontMetrics(boldFont).height();

        // caption bar background rect
        const QRectF captionRect(
                borderInnerRect.topLeft(),
                QSizeF(borderInnerRect.width(), fontHeight + padding * 2));
        captionBarItem->setRect(captionRect);
        captionBarItem->setPen(Qt::NoPen);
        captionBarItem->setBrush(color);

        captionHeight = captionRect.height();

        // node label
        nodeLabelItem->setText(getNodeLabelsString(nodeLabels));
        nodeLabelItem->setFont(boldFont);
        nodeLabelItem->setBrush(textColor);
        nodeLabelItem->setPos(captionRect.topLeft() + QPointF(padding, padding));

        // card ID
        if (cardId >= 0)
            cardIdItem->setText(QString("Card %1").arg(cardId));
        else
            cardIdItem->setText("");
        cardIdItem->setFont(normalFont);
        cardIdItem->setBrush(textColor);
        {
            const double xMin
                    = captionRect.left() + padding + nodeLabelItem->boundingRect().width() + 6.0;
            double x = captionRect.right() - padding - cardIdItem->boundingRect().size().width();
            x = std::max(x, xMin);
            cardIdItem->setPos(x, captionRect.top() + padding);
        }
    }

    //
    {
        contentsRectItem->setRect(
                borderInnerRect.marginsRemoved({0.0, captionHeight, 0.0, 0.0})); // <^>v
        contentsRectItem->setPen(Qt::NoPen);
        contentsRectItem->setBrush(Qt::white);
        contentsRectItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    }

    // title
    double yTitleBottom = 0;
    {
        constexpr int padding = 3;
        constexpr int fontPointSize = 18;
        const QColor textColor(Qt::black);

        //
        QFont font = fontOfView;
        font.setPointSize(fontPointSize);
        font.setBold(true);

        const double minHeight = QFontMetrics(font).height();

        //
        titleItem->setTextWidth(
                std::max(borderInnerRect.width() - padding * 2, 0.0));
        titleItem->setFont(font);
        titleItem->setDefaultTextColor(textColor);
        titleItem->setPos(
                contentsRectItem->rect().topLeft() + QPointF(padding, padding));

        yTitleBottom
                = contentsRectItem->rect().top()
                  + std::max(titleItem->boundingRect().height(), minHeight)
                  + padding * 2;
    }

    // text
    {
        constexpr int leftPadding = 3;
        constexpr int fontPointSize = 12;

        //
        const double height = contentsRectItem->rect().bottom() - yTitleBottom;
        if (height < 0.1) {
            textEditProxyWidget->setVisible(false);
        }
        else {
            textEditProxyWidget->resize(contentsRectItem->rect().width() - leftPadding, height);
            textEditProxyWidget->setVisible(true);
        }

        textEditProxyWidget->setPos(contentsRectItem->rect().left() + leftPadding, yTitleBottom);

        textEdit->setFrameShape(QFrame::NoFrame);
        textEdit->setMinimumHeight(10);
        textEdit->setContextMenuPolicy(Qt::NoContextMenu);
        textEdit->setStyleSheet(
                "QTextEdit {"
                "  font-size: " + QString::number(fontPointSize) + "pt;"
                "}"
                "QScrollBar:vertical {"
                "  width: 12px;"
                "}"
        );

        // focus indicator rect
        constexpr double lineWidth = 2.0;
        textEditFocusIndicator->setRect(
                QRectF(contentsRectItem->rect().left(), yTitleBottom - 2.0,
                       contentsRectItem->rect().width(), height + 2.0)
                    .marginsRemoved(uniformMarginsF(lineWidth / 2.0))
        );
        textEditFocusIndicator->setBrush(Qt::NoBrush);
        textEditFocusIndicator->setPen(QPen(QBrush(QColor(195, 225, 255)), lineWidth));
        textEditFocusIndicator->setVisible(false);
    }
}

QGraphicsView *NodeRect::getView() {
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

QColor NodeRect::getHighlightBoxColor(const QColor &color) {
    int h, s, v;
    color.getHsv(&h, &s, &v);

    if (h >= 180 && h <= 240
            && s >= 50
            && v >= 60) {
        return QColor(90, 90, 90);
    }
    else {
        return QColor(36, 128, 220); // hue = 210
    }
}
