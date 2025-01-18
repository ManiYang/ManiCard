#include <QDebug>
#include <QKeyEvent>
#include <QMimeData>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWheelEvent>
#include "custom_text_edit.h"

CustomTextEdit::CustomTextEdit(QWidget *parent)
        : QFrame(parent)
        , textEdit(new TextEditTweak) {
    QFrame::setContextMenuPolicy(Qt::NoContextMenu);

    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(textEdit);

    textEdit->setFrameShape(QFrame::NoFrame);
    textEdit->setAcceptRichText(false);
    textEdit->installEventFilter(this);

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

void CustomTextEdit::clear(const bool resetFormat) {
    textChangeIsByUser = false;

    //
    textEdit->clear();
    if (resetFormat)
        textEdit->setCurrentCharFormat({});

    //
    textChangeIsByUser = true;
}

void CustomTextEdit::setPlainText(const QString &text) {
    textChangeIsByUser = false;
    textEdit->setPlainText(text);
    textChangeIsByUser = true;
}

void CustomTextEdit::setMarkdown(const QString &text) {
    textChangeIsByUser = false;
    textEdit->setMarkdown(text);
    textChangeIsByUser = true;
}

void CustomTextEdit::setReadOnly(const bool readonly) {
    textEdit->setReadOnly(readonly);
}

void CustomTextEdit::enableSetEveryWheelEventAccepted(const bool enable){
    textEdit->enableSetEveryWheelEventAccepted(enable);
}

void CustomTextEdit::obtainFocus() {
    textEdit->setFocus();
}

void CustomTextEdit::setLineHeightPercent(const int percentage) {
    textChangeIsByUser = false;

    //
    auto cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    while (true) {
        auto blockFormat = cursor.blockFormat();
        blockFormat.setLineHeight(percentage, QTextBlockFormat::ProportionalHeight);

        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor); // select the block
        cursor.setBlockFormat(blockFormat);

        bool ok = cursor.movePosition(QTextCursor::NextBlock);
        if (!ok)
            break;
    }

    //
    textChangeIsByUser = true;
}

void CustomTextEdit::setParagraphSpacing(const double spacing) {
    textChangeIsByUser = false;

    //
    auto cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    while (true) {
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor); // select the block

        const bool isListItem = (cursor.block().textList() != nullptr);
        if (!isListItem) {
            auto blockFormat = cursor.blockFormat();
            blockFormat.setTopMargin(spacing / 2.0);
            blockFormat.setBottomMargin(spacing / 2.0);

            cursor.setBlockFormat(blockFormat);
        }

        // move to next block
        bool ok = cursor.movePosition(QTextCursor::NextBlock);
        if (!ok)
            break;
    }

    //
    textChangeIsByUser = true;
}

void CustomTextEdit::setTextCursorPosition(const int pos) {
    auto cursor = textEdit->textCursor();
    cursor.setPosition(pos);
    textEdit->setTextCursor(cursor);
}

void CustomTextEdit::setReplaceTabBySpaces(const int numberOfSpaces) {
    numberOfSpacesToReplaceTab = numberOfSpaces;
}

void CustomTextEdit::setContextMenuPolicy(Qt::ContextMenuPolicy policy) {
    textEdit->setContextMenuPolicy(policy);
}

QString CustomTextEdit::toPlainText() const {
    return textEdit->toPlainText();
}

QTextDocument *CustomTextEdit::document() const {
    return textEdit->document();
}

bool CustomTextEdit::isVerticalScrollBarVisible() const {
    return textEdit->verticalScrollBar()->isVisible();
}

int CustomTextEdit::currentTextCursorPosition() const {
    return textEdit->textCursor().position();
}

bool CustomTextEdit::eventFilter(QObject *watched, QEvent *event) {
    if (watched == textEdit) {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
            Q_ASSERT(keyEvent != nullptr);

            if (keyEvent->key() == Qt::Key_Tab
                    && keyEvent->modifiers() == Qt::NoModifier) { // (TAB pressed without modifier)
                if (numberOfSpacesToReplaceTab >= 0) {
                    insertSpaces(numberOfSpacesToReplaceTab);
                    return true;
                }
            }
            else if (keyEvent->key() == Qt::Key_Backtab
                    && keyEvent->modifiers() == Qt::ShiftModifier) { // (SHIFT-TAB pressed)
                // do nothing
                return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void CustomTextEdit::insertSpaces(const int count) {
    if (count <= 0)
        return;

    auto cursor = textEdit->textCursor();
    for (int i = 0; i < count; ++i)
        cursor.insertText(" ");
    textEdit->setTextCursor(cursor);
}

//====

TextEditTweak::TextEditTweak(QWidget *parent)
    : QTextEdit(parent) {
}

void TextEditTweak::enableSetEveryWheelEventAccepted(const bool enable) {
    setEveryWheelEventAccepted = enable;
}

void TextEditTweak::wheelEvent(QWheelEvent *event) {
    QTextEdit::wheelEvent(event);

    if (setEveryWheelEventAccepted)
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

void TextEditTweak::insertFromMimeData(const QMimeData *source) {
    if (source->hasText())
        QTextEdit::insertPlainText(source->text());
    else
        QTextEdit::insertFromMimeData(source);
}
