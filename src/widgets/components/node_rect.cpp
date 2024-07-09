#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QPainter>
#include <QPen>
#include <QGraphicsView>
#include "node_rect.h"

NodeRect::NodeRect(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , captionBarItem(new QGraphicsRectItem(this))
    , nodeLabelItem(new QGraphicsSimpleTextItem(this))
    , cardIdItem(new QGraphicsSimpleTextItem(this))
{
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    adjustChildItems();
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

void NodeRect::setBorderWidth(const double width) {
    borderWidth = width;
    redraw();
}

void NodeRect::setNodeLabel(const QString &label) {
    nodeLabel = label;
    adjustChildItems();
}

void NodeRect::setCardId(const int cardId_) {
    cardId = cardId_;
    adjustChildItems();
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
    painter->drawRoundedRect(enclosingRect, borderWidth, borderWidth);

    //
    painter->restore();
}

void NodeRect::adjustChildItems() {
    // get view's font
    QFont fontOfView;
    if (scene() != nullptr) {
        if (const auto views = scene()->views(); !views.isEmpty()) {
            auto *view = views.at(0);
            fontOfView = view->font();
        }
    }

    //
    const auto borderInnerRect = enclosingRect.marginsRemoved(
            {borderWidth, borderWidth, borderWidth, borderWidth});

    // caption bar
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

        // node label
        nodeLabelItem->setText(nodeLabel);
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

    // title




    // text




}

