#ifndef APP_STYLE_SHEET_H
#define APP_STYLE_SHEET_H

#include <QString>
#include "utilities/style_sheet_util.h"

namespace StyleClass {
constexpr char highContrastBackground[] = "highContrastBackground";
constexpr char flatToolButton[] = "flatToolButton";
constexpr char flatPushButton[] = "flatPushButton";
} // namespace StyleClass

QString getLightThemeStyleSheet();
QString getDarkThemeStyleSheet();

#endif // APP_STYLE_SHEET_H
