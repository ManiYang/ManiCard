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
#include "models/node_labels.h"
#include "node_rect.h"
#include "utilities/margins_util.h"
#include "widgets/components/custom_text_edit.h"
#include "widgets/components/graphics_item_move_resize.h"
#include "widgets/components/custom_graphics_text_item.h"

constexpr int savingInterval = 2500;

NodeRect::NodeRect(QGraphicsItem *parent)
        : QGraphicsObject(parent)
        , captionBarItem(new QGraphicsRectItem(this))
        , nodeLabelItem(new QGraphicsSimpleTextItem(this))
        , cardIdItem(new QGraphicsSimpleTextItem(this))
        , contentsRectItem(new QGraphicsRectItem(this))
        , titleItem(new CustomGraphicsTextItem(contentsRectItem))
        , textEdit(new CustomTextEdit(nullptr))
        , textEditProxyWidget(new QGraphicsProxyWidget(contentsRectItem))
        , moveResizeHelper(new GraphicsItemMoveResize(this))
        , propertiesSaving(new SavingDebouncer(savingInterval, this)) {
    textEdit->setVisible(false);
    textEdit->setReadOnly(true);
    textEditProxyWidget->setWidget(textEdit);

    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    setAcceptHoverEvents(true);

    setUpConnections();
    adjustChildItems();
}

void NodeRect::initialize() {
    Q_ASSERT(scene() != nullptr);

    moveResizeHelper->setMoveHandle(captionBarItem);

    constexpr double resizeAreaMaxWidth = 6.0;
    moveResizeHelper->setResizeHandle(this, resizeAreaMaxWidth, minSizeForResizing);
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

void NodeRect::setNodeLabels(const QSet<QString> &labels) {
    nodeLabels = labels;
    adjustChildItems();
}

void NodeRect::setCardId(const int cardId_) {
    cardId = cardId_;
    adjustChildItems();
}

void NodeRect::setTitle(const QString &title_) {
    titleItem->setPlainText(title_);
    adjustChildItems();
}

void NodeRect::setText(const QString &text_) {
    textEdit->setPlainText(text_);
    adjustChildItems();
}

void NodeRect::setEditable(const bool editable) {
    isEditable = editable;

    titleItem->setEditable(editable);
    textEdit->setReadOnly(!isEditable);
}

int NodeRect::getCardId() const {
    return cardId;
}

QString NodeRect::getTitle() const {
    return titleItem->toPlainText();
}

QString NodeRect::getText() const {
    return textEdit->toPlainText();
}

QRectF NodeRect::boundingRect() const {
    return enclosingRect;
}

void NodeRect::paint(
        QPainter *painter, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    painter->save();

    // background rect
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    const double radius = borderWidth;
    painter->drawRoundedRect(
            enclosingRect.marginsRemoved(uniformMargins(marginWidth)),
            radius, radius);

    //
    painter->restore();
}

void NodeRect::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    event->accept();
}

void NodeRect::setUpConnections() {
    connect(titleItem, &CustomGraphicsTextItem::textEdited, this, [this](bool heightChanged) {
        titleEdited = true;
        propertiesSaving->setUpdated();
        if (heightChanged)
            adjustChildItems();
    });

    //
    connect(textEdit, &CustomTextEdit::textEdited, this, [this]() {
        textEdited = true;
        propertiesSaving->setUpdated();
    });

    // ==== moveResizeHelper ====

    connect(moveResizeHelper, &GraphicsItemMoveResize::getTargetItemPosition,
            this, [this](QPointF *pos) {
        *pos = enclosingRect.topLeft();
    }, Qt::DirectConnection);

    connect(moveResizeHelper, &GraphicsItemMoveResize::setTargetItemPosition,
            this, [this](const QPointF &pos) {
        prepareGeometryChange();
        enclosingRect.moveTopLeft(pos);
        redraw();

        scene()->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
        // this is to deal with the QGraphicsView problem
        // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::movingEnded, this, [this]() {
        emit movedOrResized();
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
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::resizingEnded, this, [this]() {
        emit movedOrResized();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::setCursorShape,
            this, [this](const std::optional<Qt::CursorShape> cursorShape) {
        if (cursorShape.has_value())
            setCursor(cursorShape.value());
        else
            unsetCursor();
    });

    // ==== propertiesSaving ====

    connect(propertiesSaving, &SavingDebouncer::saveCurrentState, this, [this]() {
        // [test]
        if (titleEdited)
            qDebug() << "save title...";

        if (textEdited)
            qDebug() << "save text...";

        QTimer::singleShot(100, this, [this]() {
           propertiesSaving->savingFinished();
        });





        //
        titleEdited = false;
        textEdited = false;
    });

    connect(propertiesSaving, &SavingDebouncer::savingScheduled, this, [this]() {
        qDebug() << "saving scheduled";


    });

    connect(propertiesSaving, &SavingDebouncer::cleared, this, [this]() {
        qDebug() << "saving cleared";


    });
}

void NodeRect::adjustChildItems() {
    qDebug().noquote() << "adjustChildItems()";

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
            = enclosingRect.marginsRemoved(uniformMargins(marginWidth + borderWidth));

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
                borderInnerRect.marginsRemoved({0.0, captionHeight, 0.0, 0.0})); // L,T,R,B
        contentsRectItem->setPen(Qt::NoPen);
        contentsRectItem->setBrush(Qt::white);
        contentsRectItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    }

    // title
    double titleBottom = 0;
    {
        constexpr int padding = 3;
        constexpr int fontPointSize = 18;
        const QString fontFamily = "Arial";
        const QColor textColor(Qt::black);

        //
        QFont font = fontOfView;
        font.setFamily(fontFamily);
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

        titleBottom
                = contentsRectItem->rect().top()
                  + std::max(titleItem->boundingRect().height(), minHeight)
                  + padding * 2;
    }

    // text
    {
        constexpr int leftPadding = 3;
        constexpr int fontPointSize = 12;
        const QString fontFamily = "Arial";

        //
        QFont font = fontOfView;
        font.setFamily(fontFamily);
        font.setPointSize(fontPointSize);

        //
        const double height = contentsRectItem->rect().bottom() - titleBottom;
        if (height < 0.1) {
            textEditProxyWidget->setVisible(false);
        }
        else {
            textEditProxyWidget->resize(contentsRectItem->rect().width() - leftPadding, height);
            textEditProxyWidget->setFont(font);
            textEditProxyWidget->setVisible(true);
        }

        textEditProxyWidget->setPos(contentsRectItem->rect().left() + leftPadding, titleBottom);

        textEdit->setFrameShape(QFrame::NoFrame);
        textEdit->setContextMenuPolicy(Qt::NoContextMenu);
        textEdit->setStyleSheet(
                "QTextEdit {"
                "  font-size: "+QString::number(fontPointSize)+"pt;"
                "}"
                "QScrollBar:vertical {"
                "  width: 12px;"
                "}"
        );
    }
}

QString NodeRect::getNodeLabelsString(const QSet<QString> &labels) {
    QStringList nodeLabelsList(labels.constBegin(), labels.constEnd());
    std::sort(nodeLabelsList.begin(), nodeLabelsList.end());

    for (QString &label: nodeLabelsList) {
        label = ":" + label;
    }

    return nodeLabelsList.join(" ");
}
