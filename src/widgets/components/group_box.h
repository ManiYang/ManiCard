#ifndef GROUPBOX_H
#define GROUPBOX_H

#include "widgets/components/board_box_item.h"
#include "widgets/icons.h"

class GroupBox : public BoardBoxItem
{
    Q_OBJECT
public:
    explicit GroupBox(const int groupBoxId, QGraphicsItem *parent = nullptr);

    // Call these "set" methods only after this item is initialized:
    void setTitle(const QString &title);

    //
    QString getTitle() const;

signals:
    void leftButtonPressed();
    void ctrlLeftButtonPressedOnCaptionBar();

    void userToRenameGroupBox();
    void userToRemoveGroupBox();

private:
    static BoardBoxItem::CreationParameters getCreationParameters();

    const int groupBoxId;
    QString title;

    QHash<QAction *, Icon> contextMenuActionToIcon;

    //
    QMenu *createCaptionBarContextMenu() override;
    void adjustCaptionBarContextMenuBeforePopup(QMenu *contextMenu) override;
    void setUpContents(QGraphicsItem *contentsContainer) override;
    void adjustContents() override;
    void onMouseLeftPressed(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;
    void onMouseLeftClicked(const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) override;
};

#endif // GROUPBOX_H
