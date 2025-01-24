#include "app_data_readonly.h"
#include "services.h"
#include "setting_box.h"
#include "utilities/json_util.h"
#include "widgets/widgets_constants.h"
#include "widgets/components/custom_graphics_text_item.h"
#include "widgets/components/custom_text_edit.h"

SettingBox::SettingBox(QGraphicsItem *parent)
    : BoardBoxItem(CreationParameters {}, parent) {
}

SettingBox::~SettingBox() {
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

void SettingBox::setTitle(const QString &title) {
    titleItem->setPlainText(title);
    adjustContents();
}

void SettingBox::setDescription(const QString &description) {
    descriptionItem->setPlainText(description);
    adjustContents();
}

void SettingBox::setSettingJson(const QJsonObject &json) {
    constexpr bool compact = false;
    textEdit->setPlainText(printJson(json, compact));
    textEdit->setTextCursorPosition(0);
    adjustContents();
}

void SettingBox::setTextEditorIgnoreWheelEvent(const bool b) {
    textEditIgnoreWheelEvent = b;
}

bool SettingBox::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
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

QMenu *SettingBox::createCaptionBarContextMenu() {
    return nullptr;
}

void SettingBox::adjustCaptionBarContextMenuBeforePopup(QMenu */*contextMenu*/) {
    // do nothing
}

void SettingBox::setUpContents(QGraphicsItem *contentsContainer) {
    titleItem = new CustomGraphicsTextItem(contentsContainer);
    descriptionItem = new CustomGraphicsTextItem(contentsContainer);

    textEdit = new CustomTextEdit(nullptr);
    textEditProxyWidget = new QGraphicsProxyWidget(contentsContainer);
    textEdit->setVisible(false);
    textEditProxyWidget->setWidget(textEdit);

    //
    titleItem->setEditable(false);
    descriptionItem->setEditable(false);
    textEdit->setReadOnly(false);

    // get view's font
    QFont fontOfView;
    if (QGraphicsView *view = getView(); view != nullptr)
        fontOfView = view->font();

    //
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    const QColor titleTextColor = getTitleItemDefaultTextColor(isDarkTheme);

    // categoryNameItem
    {
        QFont font = fontOfView;
        font.setPixelSize(18);
        font.setBold(true);

        titleItem->setFont(font);
        titleItem->setDefaultTextColor(titleTextColor);
    }

    // descriptionItem
    {
        QFont font = fontOfView;
        font.setPixelSize(13);

        descriptionItem->setFont(font);
        descriptionItem->setDefaultTextColor(titleTextColor);
    }

    // textEdit
    textEdit->enableSetEveryWheelEventAccepted(true);
    textEdit->setFrameShape(QFrame::NoFrame);
    textEdit->setMinimumHeight(10);
    textEdit->setContextMenuPolicy(Qt::NoContextMenu);

    constexpr int textEditFontPixelSize = 15;
    const QString fontFamily = qApp->property("monospaceFontFamily").toString();
    textEdit->setStyleSheet(
            "QTextEdit {"
            "  font-family: \"" + fontFamily + "\";"
            "  font-size: " + QString::number(textEditFontPixelSize) + "px;" +
            "}"
            "QScrollBar:vertical {"
            "  width: 12px;"
            "}"
    );

    //
    constexpr bool bold = true;
    setCaptionBarLeftText("Setting", bold);

    // ==== install event filter ====

    textEditProxyWidget->installSceneEventFilter(this);

    // ==== set up connections ====

    // textEdit
    connect(textEdit, &CustomTextEdit::textEdited, this, [this]() {
            const QString plainText = textEdit->toPlainText();
//            emit titleTextUpdated(std::nullopt, plainText);
    });

    connect(textEdit, &CustomTextEdit::clicked, this, [this]() {
        emit leftButtonPressedOrClicked();
    });

    //
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {
        titleItem->setDefaultTextColor(getTitleItemDefaultTextColor(isDarkTheme));
        descriptionItem->setDefaultTextColor(getTitleItemDefaultTextColor(isDarkTheme));
    });
}

void SettingBox::adjustContents() {
    const QRectF contentsRect = getContentsRect();

    // categoryNameItem
    double yBottom = contentsRect.top();
    {
        constexpr double padding = 3;
        const double minHeight = QFontMetrics(titleItem->font()).height();

        //
        titleItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        titleItem->setPos(contentsRect.topLeft() + QPointF(padding, padding));

        yBottom += std::max(titleItem->boundingRect().height(), minHeight)
                   + padding * 2;
    }

    // descriptionItem
    if (descriptionItem->toPlainText().trimmed().isEmpty()) {
        descriptionItem->setVisible(false);
    }
    else {
        descriptionItem->setVisible(true);

        //
        constexpr double padding = 3;

        descriptionItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        descriptionItem->setPos(contentsRect.left() + padding, yBottom);

        yBottom += descriptionItem->boundingRect().height() + padding;
    }

    // text (settings JSON)
    {
        constexpr double leftPadding = 3;

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

void SettingBox::onMouseLeftPressed(
        const bool /*isOnCaptionBar*/, const Qt::KeyboardModifiers modifiers) {
    if (modifiers == Qt::NoModifier)
        emit leftButtonPressedOrClicked();
}

void SettingBox::onMouseLeftClicked(
        const bool /*isOnCaptionBar*/, const Qt::KeyboardModifiers /*modifiers*/) {
    // do nothing
}

QColor SettingBox::getTitleItemDefaultTextColor(const bool isDarkTheme) {
    return isDarkTheme ? QColor(darkThemeStandardTextColor) : QColor(Qt::black);
}

