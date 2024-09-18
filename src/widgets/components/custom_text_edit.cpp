#include <QDebug>
#include <QVBoxLayout>
#include <QTextCursor>
#include <QWheelEvent>
#include "custom_text_edit.h"

CustomTextEdit::CustomTextEdit(const bool acceptEveryWheelEvent, QWidget *parent)
        : QFrame(parent)
        , textEdit(new TextEditTweak(acceptEveryWheelEvent)) {
    QFrame::setContextMenuPolicy(Qt::NoContextMenu);

    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(textEdit);

    textEdit->setFrameShape(QFrame::NoFrame);
    textEdit->setAcceptRichText(false);

    connect(textEdit, &TextEditTweak::textChanged, this, [this]() {
        if (textChangeIsByUser)
            emit textEdited();
    });

    connect(textEdit, &TextEditTweak::mouseReleased, this, [this]() {
        emit clicked();
    });

    connect(textEdit, &TextEditTweak::focusedIn, this, [this]() {
        emit focusedIn();
    });

    connect(textEdit, &TextEditTweak::focusedOut, this, [this]() {
        emit focusedOut();
    });
}

void CustomTextEdit::clear() {
    textChangeIsByUser = false;
    textEdit->clear();
    textChangeIsByUser = true;
}

void CustomTextEdit::setPlainText(const QString &text) {
    textChangeIsByUser = false;
    textEdit->setPlainText(text);
    textChangeIsByUser = true;
}

void CustomTextEdit::setReadOnly(const bool readonly) {
    textEdit->setReadOnly(readonly);
}

void CustomTextEdit::setContextMenuPolicy(Qt::ContextMenuPolicy policy) {
    textEdit->setContextMenuPolicy(policy);
}

QTextCursor CustomTextEdit::textCursor() const {
    return textEdit->textCursor();
}

void CustomTextEdit::setTextCursor(const QTextCursor &cursor) {
    textEdit->setTextCursor(cursor);
}

QString CustomTextEdit::toPlainText() const {
    return textEdit->toPlainText();
}

QTextDocument *CustomTextEdit::document() const {
    return textEdit->document();
}

//====

TextEditTweak::TextEditTweak(const bool acceptEveryWheelEvent_, QWidget *parent)
        : QTextEdit(parent)
        , acceptEveryWheelEvent(acceptEveryWheelEvent_) {
}

void TextEditTweak::wheelEvent(QWheelEvent *event) {
    QTextEdit::wheelEvent(event);

    if (acceptEveryWheelEvent)
        event->accept();
}

void TextEditTweak::focusInEvent(QFocusEvent *event) {
    QTextEdit::focusInEvent(event);
    emit focusedIn();
}

void TextEditTweak::focusOutEvent(QFocusEvent *event) {
    auto cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        setTextCursor(cursor);
    }

    QTextEdit::focusOutEvent(event);
    emit focusedOut();
}

void TextEditTweak::mouseReleaseEvent(QMouseEvent *event) {
    QTextEdit::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
        emit mouseReleased();
}
