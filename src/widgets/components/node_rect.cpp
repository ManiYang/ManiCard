#include <QBrush>
#include <QDebug>
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
    , contentsRectItem(new QGraphicsRectItem(this))
    , titleItem(new GraphicsTextItem(this))
    , textEdit(new TextEdit(nullptr))
    , textEditProxyWidget(new QGraphicsProxyWidget(this))
{
    textEdit->setVisible(false);
    textEdit->setReadOnly(true);
    textEditProxyWidget->setWidget(textEdit);

    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    setUpConnections();
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

void NodeRect::setTitle(const QString &title_) {
    title = title_;
    adjustChildItems();
}

void NodeRect::setText(const QString &text_) {
    text = text_;
    adjustChildItems();
}

void NodeRect::setEditable(const bool editable) {
    isEditable = editable;

    titleItem->setTextInteractionFlags(
            isEditable ? Qt::TextEditorInteraction : Qt::NoTextInteraction);
    textEdit->setReadOnly(!isEditable);
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

void NodeRect::setUpConnections() {
    connect(titleItem, &GraphicsTextItem::contentChanged, this, [this](bool heightChanged) {
        if (!handleTitleItemContentChanged)
            return;

        const QString newTitle = titleItem->toPlainText();
        if (newTitle != title)
            title = newTitle;

        if (heightChanged) {
            constexpr bool setTitleText = false;
            adjustChildItems(setTitleText);
        }
    });
}

void NodeRect::adjustChildItems(const bool setTitleText) {
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
    const auto borderInnerRect = enclosingRect.marginsRemoved(
            {borderWidth, borderWidth, borderWidth, borderWidth});

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

    //
    {
        contentsRectItem->setRect(
                borderInnerRect.marginsRemoved({0.0, captionHeight, 0.0, 0.0}));
        contentsRectItem->setPen(Qt::NoPen);
        contentsRectItem->setBrush(Qt::white);
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
        handleTitleItemContentChanged = false;
        titleItem->setTextWidth(
                std::max(borderInnerRect.width() - padding * 2, 0.0));
        if (setTitleText)
            titleItem->setPlainText(title);
        titleItem->setFont(font);
        titleItem->setDefaultTextColor(textColor);
        titleItem->setPos(
                contentsRectItem->rect().topLeft() + QPointF(padding, padding));
        handleTitleItemContentChanged = true;

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
            textEdit->setPlainText(text);
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
