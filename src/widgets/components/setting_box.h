#ifndef SETTINGBOX_H
#define SETTINGBOX_H

#include <QGraphicsProxyWidget>
#include "widgets/components/board_box_item.h"
#include "widgets/icons.h"

class CustomGraphicsTextItem;
class CustomTextEdit;

class SettingBox : public BoardBoxItem
{
    Q_OBJECT
public:
    explicit SettingBox(QGraphicsItem *parent = nullptr);
    ~SettingBox();

    void setTitle(const QString &title);
    void setDescription(const QString &description);
    void setSchema(const QString &schema);
    void setSettingJson(const QString &jsonStr);
    void setTextEditorIgnoreWheelEvent(const bool b);
    void setErrorMsg(const QString &msg);

    //
    QString getSettingText() const;

signals:
    void settingEdited();
    void closeByUser();
    void leftButtonPressedOrClicked();

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    bool textEditIgnoreWheelEvent {false};

    // content items
    // -- title & description
    CustomGraphicsTextItem *titleItem {nullptr};
    CustomGraphicsTextItem *descriptionItem {nullptr};
    // -- schema
    QGraphicsSimpleTextItem *labelSchema {nullptr};
    CustomGraphicsTextItem *schemaItem {nullptr};
    // -- setting
    QGraphicsSimpleTextItem *labelSetting {nullptr};
    CustomTextEdit *textEdit {nullptr};
            // (Use QTextEdit rather than QGraphicsTextItem. The latter does not have scrolling
            // functionality.)
    QGraphicsProxyWidget *textEditProxyWidget {nullptr};
    CustomGraphicsTextItem *settingErrorMsgItem {nullptr};

    //
    QHash<QAction *, Icon> contextMenuActionToIcon;

    //
    QMenu *createCaptionBarContextMenu() override;
    void adjustCaptionBarContextMenuBeforePopup(QMenu *contextMenu) override;
    void setUpContents(QGraphicsItem *contentsContainer) override;
    void adjustContents() override;
    void onMouseLeftPressed(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;
    void onMouseLeftClicked(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;

    // tools
    static QColor getTitleItemDefaultTextColor(const bool isDarkTheme);
};

#endif // SETTINGBOX_H
