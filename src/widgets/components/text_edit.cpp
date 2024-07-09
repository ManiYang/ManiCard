#include <QTextCursor>
#include <QWheelEvent>
#include "text_edit.h"

TextEdit::TextEdit(QWidget *parent)
    : QTextEdit(parent)
{}

void TextEdit::wheelEvent(QWheelEvent *event) {
    QTextEdit::wheelEvent(event);
    event->accept();
}

void TextEdit::focusOutEvent(QFocusEvent *event) {
    auto cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        setTextCursor(cursor);
    }

    QTextEdit::focusOutEvent(event);
}
