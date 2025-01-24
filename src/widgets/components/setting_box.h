#ifndef SETTINGBOX_H
#define SETTINGBOX_H

#include <QGraphicsProxyWidget>
#include "widgets/components/board_box_item.h"

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
    void setSettingJson(const QJsonObject &json);

    void setTextEditorIgnoreWheelEvent(const bool b);

signals:
    void leftButtonPressedOrClicked();

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    bool textEditIgnoreWheelEvent {false};

    // content items
    CustomGraphicsTextItem *titleItem {nullptr};
    CustomGraphicsTextItem *descriptionItem {nullptr};

    CustomTextEdit *textEdit;
    QGraphicsProxyWidget *textEditProxyWidget;

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
