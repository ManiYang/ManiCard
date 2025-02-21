#include <QDebug>
#include <QString>
#include "icons.h"
#include "utilities/numbers_util.h"

namespace {
QIcon createIconForDarkTheme(const QString &imageFileForLightTheme);
std::pair<QPixmap, QPixmap> createPixmapsForDarkTheme(const QPixmap &pixmapForLightTheme);
}

namespace Icons {
//!
//! \return an image file with black foreground and transparent background
//!
QString getImageFileForLightTheme(const Icon icon) {
    switch (icon)     {
    case Icon::Add:
        return ":/icons/add_black_24";
    case Icon::AddBox:
        return ":/icons/add_box_black_24";
    case Icon::OpenInNew:
        return ":/icons/open_in_new_black_24";
    case Icon::EditSquare:
        return ":/icons/edit_square_black_24";
    case Icon::ContentCopy:
        return ":/icons/content_copy_24";
    case Icon::FileSave:
        return ":/icons/file_save_24";
    case Icon::CloseBox:
        return ":/icons/close_box_black_24";
    case Icon::Delete:
        return ":/icons/delete_black_24";
    case Icon::ArrowSouth:
        return ":/icons/arrow_downward_24";
    case Icon::ArrowEast:
        return ":/icons/arrow_east_24";
    case Icon::ArrowNorth:
        return ":/icons/arrow_upward_24";
    case Icon::ArrowWest:
        return ":/icons/arrow_west_24";
    case Icon::ArrowRight:
        return ":/icons/arrow_right_black_24";
    case Icon::Menu4:
        return ":/icons/menu4_black_24";
    case Icon::MoreVert:
        return ":/icons/more_vert_24";
    case Icon::CloseRightPanel:
        return ":/icons/close_right_panel_24";
    case Icon::OpenRightPanel:
        return ":/icons/open_right_panel_24";
    case Icon::Folder:
        return ":/icons/folder_24";
    case Icon::Label:
        return ":/icons/label_black_24";
    case Icon::PlayArrow:
        return ":/icons/play_arrow_24";
    case Icon::Search:
        return ":/icons/search_72";
    }
    Q_ASSERT(false); // case not implemented
    return "";
}

QIcon getIcon(const Icon icon, const Theme theme) {
    const QString imageFileForLightTheme = getImageFileForLightTheme(icon);
    if (imageFileForLightTheme.isEmpty())
        return QIcon();

    switch (theme) {
    case Theme::Light:
        return QIcon(imageFileForLightTheme);

    case Theme::Dark:
        return createIconForDarkTheme(imageFileForLightTheme);
    }
    Q_ASSERT(false); // case not implemented
    return QIcon();
}

QPixmap getPixmap(const Icon icon, const Theme theme, const int size) {
    const QString imageFileForLightTheme = getImageFileForLightTheme(icon);
    if (imageFileForLightTheme.isEmpty())
        return QPixmap();

    //
    auto pixmap = QPixmap(imageFileForLightTheme);
    if (pixmap.width() != size)
        pixmap = pixmap.scaledToWidth(size, Qt::SmoothTransformation);

    //
    switch (theme) {
    case Theme::Light:
        return pixmap;

    case Theme::Dark:
        return createPixmapsForDarkTheme(pixmap).first;
    }
    Q_ASSERT(false); // case not implemented
    return QPixmap();
}

} // namespace Icons

namespace {
//!
//! \param imageFileForLightTheme: must have black foreground and transparent background
//! \return an icon with white foreground and transparent background
//!
QIcon createIconForDarkTheme(const QString &imageFileForLightTheme) {
    const auto [pixmapNormalMode, pixmapDisabledMode]
            = createPixmapsForDarkTheme(QPixmap(imageFileForLightTheme));

    QIcon icon(pixmapNormalMode);
    icon.addPixmap(pixmapDisabledMode, QIcon::Disabled);
    return icon;
}

//!
//! \param pixmapForLightTheme: must have black foreground and transparent background
//! \return (pixmap-for-normal-mode, pixmap-for-disabled-mode)
//!
std::pair<QPixmap, QPixmap> createPixmapsForDarkTheme(const QPixmap &pixmapForLightTheme) {
    QImage image = pixmapForLightTheme.toImage();
    if (image.isNull()) {
        qWarning().noquote() << QString("could not load image from pixmap");
        return {QPixmap(), QPixmap()};
    }

    if (image.format() != QImage::Format_ARGB32)
        image = image.convertToFormat(QImage::Format_ARGB32);

    // revert the gray scale
    const int length = image.size().width() * image.size().height();

    {
        constexpr double brightness = 0.82; // must be <= 1 and > 0
        QRgb *pixels = reinterpret_cast<QRgb *>(image.scanLine(0));
        for (int i = 0; i < length; ++i) {
            const QRgb pixel = pixels[i];
            const int r = qRed(pixel);
            const int g = qGreen(pixel);
            const int b = qBlue(pixel);
            const int a = qAlpha(pixel);
            pixels[i] = qRgba(
                    nearestInteger((255 - r) * brightness),
                    nearestInteger((255 - g) * brightness),
                    nearestInteger((255 - b) * brightness),
                    a
            );
        }
    }

    const QPixmap pixmapNormalMode = QPixmap::fromImage(image);

    // disabled mode
    {
        constexpr double brightnessRatio = 0.6; // must be <= 1 and > 0
        QRgb *pixels = reinterpret_cast<QRgb *>(image.scanLine(0));
        for (int i = 0; i < length; ++i) {
            const QRgb pixel = pixels[i];
            const int r = qRed(pixel);
            const int g = qGreen(pixel);
            const int b = qBlue(pixel);
            const int a = qAlpha(pixel);
            pixels[i] = qRgba(
                    nearestInteger(r * brightnessRatio),
                    nearestInteger(g * brightnessRatio),
                    nearestInteger(b * brightnessRatio),
                    a
            );
        }
    }

    const QPixmap pixmapDisabledMode = QPixmap::fromImage(image);

    //
    return {pixmapNormalMode, pixmapDisabledMode};
}
} // namespace
