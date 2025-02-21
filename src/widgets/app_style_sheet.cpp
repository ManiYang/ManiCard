#include "app_style_sheet.h"
#include "widgets/widgets_constants.h"

namespace {
QString getBaseStyleSheet() {
    return
        "* {"
        "  font-size: 10pt;"
        "}"

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
        "  margin-left: -1px;" // expand/overlap to the left and right by 2px
        "  margin-right: -1px;"
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

        "QTableWidget::item:selected {"
        "  color: black;"
        "  background-color: #d0d0d0;"
        "}"
        "QHeaderView::section {"
        "  background-color: #f0f0f0;"
        "  border: 1px solid #e0e0e0;"
        "}"

        "QTabBar::tab {"
        "  border: 1px solid #d9d9d9;"
        "  background: #f0f0f0;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #e6e6e6;" // same as board background
        "  border-bottom-color: #e6e6e6;" // same as board background
        "}"

        "*" + styleClassSelector(StyleClass::mediumContrastTextColor) + " {" +
        "  color: #606060;"
        "}"

        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatToolButton) + " {"  +
        "  background: #e0e0e0;"
        "}"
        "QAbstractButton:checked" + styleClassSelector(StyleClass::flatToolButton) + " {"  +
        "  background: #e0e0e0;"
        "}"

        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #e0e0e0;"
        "}"
        "QAbstractButton:pressed" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #c0c0c0;"
        "}"

        "QFrame" + styleClassSelector(StyleClass::frameWithSolidBorder) + " {" +
        "  border: 1px solid #c0c0c0;"
        "}"

        "";
}

QString getDarkThemeStyleSheet() {
    return getBaseStyleSheet() +
        "* {"
        "  color: " + darkThemeStandardTextColor + ";" +
        "  background: #0f1114;"
        "}"
        "QLabel, QCheckBox, QRadioButton {"
        "   background: transparent;"
        "}"
        "QToolTip {"
        "  border: 1px solid #c0c0c0;"
        "  background-color: #0f1114;"
        "}"

        "QDialog {"
        "  background: #383838;"
        "}"
        "QDialog QPushButton {"
        "  background: #606060;"
        "}"
        "QDialog QPushButton:hover {"
        "  background: #808080;"
        "}"

        "QSplitter::handle {"
        "  background: #2b323b;"
        "}"

        "QMenu {"
        "  color: white;"
        "  background: #383838;"
        "}"
        "QMenu::item:selected, QListWidget::item:hover {"
        "  background: #235c96;"
        "}"
        "QListWidget::item:selected {"
        "  background: #4b739b;"
        "}"

        "QTextEdit, QListView, QLineEdit {"
        "  background: " + highContrastDarkBackgroundColor + ";" +
        "}"
        "QLineEdit, QComboBox {"
        "  border: 1px solid #828790;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #aeb1b7;"
        "}"

        "QComboBox:!editable {"
        "  background: #373f49;"
        "}"

        "QTableView {"
        "  gridline-color: #505050;"
        "}"
        "QTableWidget::item:selected {"
        "  color: white;"
        "  background-color: #505050;"
        "}"
        "QHeaderView::section {"
        "  background-color: #383838;"
        "  border: 1px solid #505050;"
        "}"

        "QScrollBar {"
        "  border: 1px solid #2c323a;"
        "  background: #2c323a;"
        "}"
        "QScrollBar::handle, QScrollBar::add-page, QScrollBar::sub-page {"
        "  background: none;"
        "}"
        "QScrollBar::handle, QScrollBar::add-line, QScrollBar::sub-line {"
        "  border: none;"
        "}"
        "QScrollBar::handle {"
        "  background: #424a57;"
        "}"

        "QTabBar::tab {"
        "  border: 1px solid #424242;"
        "  background: #0f1114;"
        "}"
        "QTabBar::tab:selected {"
        "  background: " + darkThemeBoardBackground + ";" +
        "  border-bottom-color: " + darkThemeBoardBackground + ";" +
        "}"

        "*" + styleClassSelector(StyleClass::highContrastBackground) + " {" +
        "  background: " + highContrastDarkBackgroundColor + ";" +
        "}"

        "*" + styleClassSelector(StyleClass::mediumContrastTextColor) + " {" +
        "  color: #b0b0b0;"
        "}"

        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatToolButton) + " {" +
        "  background: #484848;"
        "}"
        "QAbstractButton:checked" + styleClassSelector(StyleClass::flatToolButton) + " {" +
        "  background: #484848;"
        "}"

        "QAbstractButton:hover" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #484848;"
        "}"
        "QAbstractButton:pressed" + styleClassSelector(StyleClass::flatPushButton) + " {" +
        "  background: #5a5a5a;"
        "}"

        "QFrame" + styleClassSelector(StyleClass::frameWithSolidBorder) + " {" +
        "  border: 1px solid #5a5a5a;"
        "}"

        "";
}
