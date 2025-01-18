#include "app_style_sheet.h"

namespace {
QString getBaseStyleSheet() {
    return
        "QAbstractButton" + styleClassSelector(StyleClass::flatToolButton) + " {"  +
        "  border: none;"
        "  background: transparent;"
        "}"

        "QAbstractButton" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 2px 4px 2px 2px;"
        "  background: transparent;"
        "}"

        "";
};
} // namespace

QString getLightThemeStyleSheet() {
    return getBaseStyleSheet() +
        "*" + styleClassSelector(StyleClass::highContrastBackground) + " {" +
        "  background: white;"
        "}"

        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatToolButton) + " {"  +
        "  background: #e0e0e0;"
        "}"

        "QAbstractButton" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  color: #606060;"
        "}"
        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #e0e0e0;"
        "}"
        "QAbstractButton:pressed" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #c0c0c0;"
        "}"

        "";
}

QString getDarkThemeStyleSheet() {
    return getBaseStyleSheet() +
        "* {"
        "  color: #e0e0e0;"
        "  background: #272727;"
        "}"

        "QMenu::item:selected, QListWidget::item:hover {"
        "  background: #235c96;"
        "}"

        "QTextEdit, QListView, QLineEdit {"
        "  background: black;"
        "}"

        "*" + styleClassSelector(StyleClass::highContrastBackground) + " {" +
        "  background: black;"
        "}"

        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatToolButton) + " {" +
        "  background: #484848;"
        "}"

        "QAbstractButton" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  color: #c0c0c0;"
        "}"
        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #484848;"
        "}"
        "QAbstractButton:pressed" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #5a5a5a;"
        "}"

        "";
}
