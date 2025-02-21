#ifndef ICONS_H
#define ICONS_H

#include <QIcon>

enum class Icon {
    Add, AddBox, OpenInNew,
    EditSquare, ContentCopy,
    FileSave,
    CloseBox, Delete,

    ArrowSouth, ArrowEast, ArrowNorth, ArrowWest,
    ArrowRight,

    Menu4, MoreVert,

    CloseRightPanel, OpenRightPanel,

    Folder,
    Label,
    PlayArrow,
    Search
};

namespace Icons {
enum class Theme {Light, Dark};
QIcon getIcon(const Icon icon, const Theme theme);
} // namespace Icons

#endif // ICONS_H
