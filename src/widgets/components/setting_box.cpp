#include <QMessageBox>
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

void SettingBox::setSchema(const QString &schema)
{
    schemaItem->setPlainText(schema);
    adjustContents();
}

void SettingBox::setSettingJson(const QString &jsonStr) {
    textEdit->setPlainText(jsonStr);
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
    QMenu *menu = new QMenu;
    {
        auto *action = menu->addAction("Close");
        contextMenuActionToIcon.insert(action, Icon::CloseBox);
        connect(action, &QAction::triggered, this, [this]() {
            const auto r = QMessageBox::question(getView(), " ", "Close the setting?");
            if (r == QMessageBox::Yes)
                emit closeByUser();
        });
    }
    return menu;
}

void SettingBox::adjustCaptionBarContextMenuBeforePopup(QMenu */*contextMenu*/) {
    // set action icons
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = contextMenuActionToIcon.constBegin();
            it != contextMenuActionToIcon.constEnd(); ++it) {
        it.key()->setIcon(Icons::getIcon(it.value(), theme));
    }
}

void SettingBox::setUpContents(QGraphicsItem *contentsContainer) {
    titleItem = new CustomGraphicsTextItem(contentsContainer);
    descriptionItem = new CustomGraphicsTextItem(contentsContainer);
    labelSchema = new QGraphicsSimpleTextItem(contentsContainer);
    schemaItem = new CustomGraphicsTextItem(contentsContainer);
    labelSetting = new QGraphicsSimpleTextItem(contentsContainer);

    textEdit = new CustomTextEdit(nullptr);
    textEditProxyWidget = new QGraphicsProxyWidget(contentsContainer);
    textEdit->setVisible(false);
    textEditProxyWidget->setWidget(textEdit);

    settingErrorMsgItem = new CustomGraphicsTextItem(contentsContainer);

    //
    titleItem->setEditable(false);
    descriptionItem->setEditable(false);
    schemaItem->setTextSelectable(true);
    textEdit->setReadOnly(false);

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

    // labels
    {
        QFont font = fontOfView;
        font.setPixelSize(13);
        font.setBold(true);

        const QColor textColor(127, 127, 127);


        labelSchema->setText("Schema:");
        labelSchema->setFont(font);
        labelSchema->setBrush(textColor);

        labelSetting->setText("Setting:");
        labelSetting->setFont(font);
        labelSetting->setBrush(textColor);
    }

    // schemaItem
    {
        QFont font = fontOfView;
        font.setPixelSize(13);
        font.setFamily(qApp->property("monospaceFontFamily").toString());

        schemaItem->setFont(font);
        schemaItem->setDefaultTextColor(titleTextColor);
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

    // settingErrorMsgItem
    {
        QFont font = fontOfView;
        font.setPixelSize(13);

        const QColor textColor(Qt::red);

        settingErrorMsgItem->setFont(font);
        settingErrorMsgItem->setDefaultTextColor(textColor);
    }

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
        const auto color = getTitleItemDefaultTextColor(isDarkTheme);
        titleItem->setDefaultTextColor(color);
        descriptionItem->setDefaultTextColor(color);
        schemaItem->setDefaultTextColor(color);
    });
}

void SettingBox::adjustContents() {
    const QRectF contentsRect = getContentsRect();

    // categoryNameItem
    double yBottom = contentsRect.top();
    {
        constexpr double topPadding = 3;
        constexpr double bottomPadding = 1;
        constexpr double xPadding = 3;
        const double minHeight = QFontMetrics(titleItem->font()).height();

        //
        titleItem->setTextWidth(
                std::max(contentsRect.width() - xPadding * 2, 0.0));
        titleItem->setPos(contentsRect.topLeft() + QPointF(xPadding, topPadding));

        yBottom += std::max(titleItem->boundingRect().height(), minHeight)
                   + topPadding + bottomPadding;
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

    // labelSchema
    {
        constexpr int xPadding = 3;
        labelSchema->setPos(contentsRect.left() + xPadding, yBottom);

        yBottom += labelSchema->boundingRect().height();
    }

    // schemaItem
    {
        constexpr int padding = 3;
        const double minHeight = QFontMetrics(schemaItem->font()).height();

        schemaItem->setTextWidth(
                std::max(contentsRect.width() - padding * 2, 0.0));
        schemaItem->setPos(contentsRect.left() + padding, yBottom + padding);

        yBottom += std::max(schemaItem->boundingRect().height(), minHeight)
                   + padding * 2;
    }

    // labelSetting
    {
        constexpr int padding = 3;
        labelSetting->setPos(contentsRect.left() + padding, yBottom);

        yBottom += labelSetting->boundingRect().height() + padding;
    }

    // settingErrorMsgItem (bottom-most)
    double yTop = contentsRect.bottom();
    if (settingErrorMsgItem->toPlainText().trimmed().isEmpty()) {
        settingErrorMsgItem->setVisible(false);
    }
    else {
        settingErrorMsgItem->setVisible(true);

        //
        constexpr int padding = 3;

        settingErrorMsgItem->setTextWidth(
                    std::max(contentsRect.width() - padding * 2, 0.0));
        const int itemHeight = settingErrorMsgItem->boundingRect().height();
        settingErrorMsgItem->setPos(
                contentsRect.left() + padding,
                std::max(contentsRect.bottom() - itemHeight, yBottom));

        yTop = settingErrorMsgItem->pos().y();
    }

    // text (setting)
    {
        constexpr double leftPadding = 3;

        const double textEditHeight = yTop - yBottom;
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

