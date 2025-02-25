#include <QDebug>
#include <QGraphicsScene>
#include <QMessageBox>
#include "app_data_readonly.h"
#include "node_rect.h"
#include "services.h"
#include "utilities/colors_util.h"
#include "utilities/margins_util.h"
#include "widgets/components/custom_graphics_text_item.h"
#include "widgets/components/custom_text_edit.h"
#include "widgets/widgets_constants.h"

constexpr double textEditLineHeightPercentage = 120;

NodeRect::NodeRect(const int cardId, QGraphicsItem *parent)
    : BoardBoxItem(getCreationParameters(), parent)
    , cardId(cardId)
    , titleItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
    , propertiesItem(new CustomGraphicsTextItem) // parent is set in setUpContents()
    , textEdit(new CustomTextEdit(nullptr))
    , textEditProxyWidget(new QGraphicsProxyWidget) // parent is set in setUpContents()
    , textEditFocusIndicator(new QGraphicsRectItem(this)){
}

NodeRect::~NodeRect() {
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

void NodeRect::setNodeLabels(const QStringList &labels) {
    nodeLabels = labels;

    constexpr bool bold = true;
    setCaptionBarLeftText(getNodeLabelsString(nodeLabels), bold);
}

void NodeRect::setTitle(const QString &title) {
    titleItem->setPlainText(title);
    adjustContents();
}

void NodeRect::setPropertiesDisplay(const QString &propertiesDisplayText) {
    propertiesItem->setPlainText(propertiesDisplayText);
    adjustContents();
}

void NodeRect::setText(const QString &text) {
    plainText = text;
    textEditIsPreviewMode = false;
    textEdit->setPlainText(text);
    textEdit->setLineHeightPercent(textEditLineHeightPercentage);
    adjustContents();

    textEdit->setTextCursorPosition(0);
}

void NodeRect::setEditable(const bool editable) {
    nodeRectIsEditable = editable;

    using TextInteractionState = CustomGraphicsTextItem::TextInteractionState;
    titleItem->setTextInteractionState(
            editable ? TextInteractionState::Editable : TextInteractionState::Selectable);

    textEdit->setReadOnly(
            !computeTextEditEditable(nodeRectIsEditable, textEditIsPreviewMode));
}

void NodeRect::setTextEditorIgnoreWheelEvent(const bool b) {
    textEditIgnoreWheelEvent = b;
}

void NodeRect::togglePreview() {
    textEditIsPreviewMode = !textEditIsPreviewMode;

    if (textEditIsPreviewMode) {
        textEditCursorPositionBeforePreviewMode = textEdit->currentTextCursorPosition();

        textEdit->setMarkdown(plainText);
        textEdit->document()->setIndentWidth(20);
        textEdit->setParagraphSpacing(20);
    }
    else {
        textEdit->clear(true);
        textEdit->setPlainText(plainText);

        textEdit->setTextCursorPosition(textEditCursorPositionBeforePreviewMode);
    }
    textEdit->setLineHeightPercent(textEditLineHeightPercentage);

    textEdit->setReadOnly(
            !computeTextEditEditable(nodeRectIsEditable, textEditIsPreviewMode));

    //
    adjustContents();
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

BoardBoxItem::CreationParameters NodeRect::getCreationParameters() {
    CreationParameters parameters;
    parameters.contentsBackgroundType = ContentsBackgroundType::Opaque;
    parameters.borderShape = BorderShape::Solid;
    parameters.highlightFrameColors = std::make_pair(QColor(36, 128, 220), QColor(46, 115, 184));

    return parameters;
}

QMenu *NodeRect::createCaptionBarContextMenu() {
    auto *contextMenu = new QMenu;
    {
        auto *action = contextMenu->addAction("Set Labels...");
        contextMenuActionToIcon.insert(action, Icon::Label);
        connect(action, &QAction::triggered, this, [this]() {
            emit userToSetLabels();
        });
    }
    {
        auto *action = contextMenu->addAction("Create Relationship...");
        contextMenuActionToIcon.insert(action, Icon::ArrowRight);
        connect(action, &QAction::triggered, this, [this]() {
            emit userToCreateRelationship();
        });
    }
    contextMenu->addSeparator();
    {
        auto *action = contextMenu->addAction("Close");
        contextMenuActionToIcon.insert(action, Icon::CloseBox);
        connect(action, &QAction::triggered, this, [this]() {
            const auto r = QMessageBox::question(getView(), " ", "Close the card?");
            if (r == QMessageBox::Yes)
                emit closeByUser();
        });
    }

    return contextMenu;
}

void NodeRect::adjustCaptionBarContextMenuBeforePopup(QMenu */*contextMenu*/) {
    // set action icons
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = contextMenuActionToIcon.constBegin();
            it != contextMenuActionToIcon.constEnd(); ++it) {
        it.key()->setIcon(Icons::getIcon(it.value(), theme));
    }
}

void NodeRect::setUpContents(QGraphicsItem *contentsContainer) {
    titleItem->setParentItem(contentsContainer);
    titleItem->setEnableContextMenu(false);

    propertiesItem->setParentItem(contentsContainer);

    using TextInteractionState = CustomGraphicsTextItem::TextInteractionState;
    propertiesItem->setTextInteractionState(TextInteractionState::Selectable);

    //
    textEditProxyWidget->setParentItem(contentsContainer);
    textEdit->setVisible(false);
    textEditProxyWidget->setWidget(textEdit);

    // get view's font
    QFont fontOfView;
    if (QGraphicsView *view = getView(); view != nullptr)
        fontOfView = view->font();

    // -- title
    {
        constexpr int fontPixelSize = 20;

        QFont font = fontOfView;
        font.setPixelSize(fontPixelSize);
        font.setBold(true);

        titleItem->setFont(font);
    }

    // -- properties
    {
        constexpr int fontPixelSize = 14;

        QFont font = fontOfView;
        font.setPixelSize(fontPixelSize);
        font.setBold(true);

        propertiesItem->setFont(font);
    }

    //
    textEdit->enableSetEveryWheelEventAccepted(true);
    textEdit->setReadOnly(!computeTextEditEditable(nodeRectIsEditable, textEditIsPreviewMode));
    textEdit->setReplaceTabBySpaces(4);
    textEdit->setFrameShape(QFrame::NoFrame);
    textEdit->setMinimumHeight(10);
    textEdit->setContextMenuPolicy(Qt::NoContextMenu);

    constexpr int textEditFontPixelSize = 16;
    textEdit->setStyleSheet(
            "QTextEdit {"
            "  font-size: " + QString::number(textEditFontPixelSize) + "px;" +
            "}"
            "QScrollBar:vertical {"
            "  width: 12px;"
            "}"
    );

    //
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();

    textEditFocusIndicator->setBrush(Qt::NoBrush);
    textEditFocusIndicator->setPen(
            getTextEditFocusIndicator(isDarkTheme, textEditFocusIndicatorLineWidth));
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
        emit leftButtonPressedOrClicked();
    });

    connect(titleItem, &CustomGraphicsTextItem::tabKeyPressed, this, [this]() {
        textEdit->obtainFocus();
    });

    // propertiesItem
    connect(propertiesItem, &CustomGraphicsTextItem::clicked, this, [this]() {
        emit leftButtonPressedOrClicked();
    });

    // textEdit
    connect(textEdit, &CustomTextEdit::textEdited, this, [this]() {
        if (!textEditIsPreviewMode) {
            plainText = textEdit->toPlainText();
            emit titleTextUpdated(std::nullopt, plainText);
        }
        //
        textEdit->setVerticalScrollBarTurnedOn(!plainText.isEmpty());
    });

    connect(textEdit, &CustomTextEdit::clicked, this, [this]() {
        emit leftButtonPressedOrClicked();
    });

    connect(textEdit, &CustomTextEdit::focusedIn, this, [this]() {
        textEditFocusIndicator->setVisible(true);
    });

    connect(textEdit, &CustomTextEdit::focusedOut, this, [this]() {
        textEditFocusIndicator->setVisible(false);
    });

    //
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {
        titleItem->setDefaultTextColor(getNormalTextColor(isDarkTheme));
        propertiesItem->setDefaultTextColor(getDimTextColor(isDarkTheme));
        textEditFocusIndicator->setPen(
                getTextEditFocusIndicator(isDarkTheme, textEditFocusIndicatorLineWidth));
    });
}

void NodeRect::adjustContents() {
    const QRectF contentsRect = getContentsRect();

    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    const QColor normalTextColor = getNormalTextColor(isDarkTheme);
    const QColor dimTextColor = getDimTextColor(isDarkTheme);

    // title
    double yBottom = 0;
    {
        constexpr int padding = 3;
        const double minHeight = QFontMetrics(titleItem->font()).height();

        titleItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        titleItem->setDefaultTextColor(normalTextColor);
        titleItem->setPos(contentsRect.topLeft() + QPointF(padding, padding));

        yBottom = contentsRect.top()
                  + std::max(titleItem->boundingRect().height(), minHeight)
                  + padding * 2;
    }

    // properties
    {
        if (propertiesItem->toPlainText().isEmpty()) {
            propertiesItem->setVisible(false);
        }
        else {
            propertiesItem->setVisible(true);

            constexpr int padding = 3;

            propertiesItem->setDefaultTextColor(dimTextColor);
            propertiesItem->setTextWidth(
                    std::max(contentsRect.width() - padding * 2, 0.0));
            propertiesItem->setPos(contentsRect.left() + padding, yBottom);

            yBottom += propertiesItem->boundingRect().height() + padding;
        }
    }

    // text
    double textEditHeight;
    {
        textEdit->setVerticalScrollBarTurnedOn(!plainText.isEmpty());

        //
        constexpr int leftPadding = 3;

        textEditHeight = contentsRect.bottom() - yBottom;
        if (textEditHeight < 0.1) {
            textEditProxyWidget->setVisible(false);
        }
        else {
            textEditProxyWidget->resize(contentsRect.width() - leftPadding, textEditHeight);
            textEditProxyWidget->setVisible(true);
        }

        textEditProxyWidget->setPos(contentsRect.left() + leftPadding, yBottom);
    }

    // textEditFocusIndicator
    textEditFocusIndicator->setRect(
            QRectF(contentsRect.left(), yBottom - 2.0,
                   contentsRect.width(), textEditHeight + 2.0)
                .marginsRemoved(uniformMarginsF(textEditFocusIndicatorLineWidth / 2.0))
    );
}

void NodeRect::onMouseLeftPressed(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) {
    if (modifiers == Qt::NoModifier) {
        emit leftButtonPressedOrClicked();
    }
    else if (modifiers == Qt::ControlModifier) {
        if (isOnCaptionBar)
            emit ctrlLeftButtonPressedOnCaptionBar();
    }
}

void NodeRect::onMouseLeftClicked(
        const bool /*isOnCaptionBar*/, const Qt::KeyboardModifiers /*modifiers*/) {
    // do nothing
}

QString NodeRect::getNodeLabelsString(const QStringList &labels) {
    QStringList labels2;
    for (const QString &label: labels)
        labels2 << (":" + label);
    return labels2.join(" ");
}

bool NodeRect::computeTextEditEditable(
        const bool nodeRectIsEditable, const bool textEditIsPreviewMode) {
    return !textEditIsPreviewMode && nodeRectIsEditable;
}

QColor NodeRect::getNormalTextColor(const bool isDarkTheme) {
    return isDarkTheme ? QColor(darkThemeStandardTextColor) : QColor(Qt::black);
}

QColor NodeRect::getDimTextColor(const bool isDarkTheme) {
    const QColor normalTextColor = getNormalTextColor(isDarkTheme);
    return shiftHslLightness(normalTextColor, (isDarkTheme ? -0.3 : 0.4));
}

QPen NodeRect::getTextEditFocusIndicator(const bool isDarkTheme, const double indicatorLineWidth) {
    const QColor color = isDarkTheme ? QColor(0, 51, 102) : QColor(195, 225, 255);
    return QPen(QBrush(color), indicatorLineWidth);
}
