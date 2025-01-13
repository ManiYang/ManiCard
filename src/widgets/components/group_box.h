#ifndef GROUPBOX_H
#define GROUPBOX_H

#include "widgets/components/board_box_item.h"

class GroupBox : public BoardBoxItem
{
    Q_OBJECT
public:
    explicit GroupBox(QGraphicsItem *parent = nullptr);

    // Call these "set" methods only after this item is initialized:
    void setTitle(const QString &title);

private:


    //
    QMenu *createCaptionBarContextMenu() override;
    void setUpContents(QGraphicsItem *contentsContainer) override;
    void adjustContents() override;
};

#endif // GROUPBOX_H