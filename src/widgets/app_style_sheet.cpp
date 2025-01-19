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

        "QMenu::item:disabled {"
        "  color: #888888;"
        "}"

        "QTabBar::tab {"
        "  padding: 4px 12px;"
        "}"
        "QTabBar::tab:!selected {"
        "  margin-top: 2px;"
        "  border-bottom-color: #909090;"
        "}"
        "QTabBar::tab:selected {"
        "  margin-left: -2px;" // expand/overlap to the left and right by 2px
        "  margin-right: -2px;"
        "}"
        "QTabBar::tab:first:selected {"
        "  margin-left: 0;" // the first selected tab has nothing to overlap with on the left
        "}"
        "QTabBar::tab:last:selected {"
        "  margin-right: 0;" // the last selected tab has nothing to overlap with on the right
        "}"
        "QTabBar::tab:only-one {"
        "  margin: 0;" // if there is only one tab, we don't want overlapping margins
        "}"

        "";
};
} // namespace

QString getLightThemeStyleSheet() {
    return getBaseStyleSheet() +
        "*" + styleClassSelector(StyleClass::highContrastBackground) + " {" +
        "  background: white;"
        "}"

        "QTabBar::tab {"
        "  border: 1px solid #d9d9d9;"
        "  background: #f0f0f0;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #e6e6e6;" // same as board background
        "  border-bottom-color: #e6e6e6;" // same as board background
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

        "QMenu {"
        "  color: white;"
        "  background: #383838;"
        "}"
        "QMenu::item:selected, QListWidget::item:hover {"
        "  background: #235c96;"
        "}"

        "QTextEdit, QListView, QLineEdit {"
        "  background: black;"
        "}"

        "QTabBar::tab {"
        "  border: 1px solid #424242;"
        "  background: #272727;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #404040;" // same as board background
        "  border-bottom-color: #404040;" // same as board background
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
