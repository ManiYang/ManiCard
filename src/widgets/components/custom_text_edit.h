#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>

class TextEditTweak;

//!
//! Wraps a QTextEdit.
//! - emits signal textEdited() only when the text is updated by user
//! - ignores Shift-Tab
//! - can be set to replace Tab with spaces
//!
class CustomTextEdit : public QFrame
{
    Q_OBJECT
public:
    //!
    //! \param acceptEveryWheelEvent: If true, every QWheelEvent is accepted after calling the
    //!             base implementation, so that the event is never passed to parent widget.
    //! \param parent
    //!
    explicit CustomTextEdit(const bool acceptEveryWheelEvent = false, QWidget *parent = nullptr);

    void clear();
    void setPlainText(const QString &text);
    void setReadOnly(const bool readonly);

    //!
    //! \param numberOfSpaces: if < 0, won't replace TAB
    //!
    void setReplaceTabBySpaces(const int numberOfSpaces);

    //!
    //! Sets context-menu policy for the wrapped QTextEdit.
    //!
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &cursor);

    QString toPlainText() const;

    //
    QTextDocument *document() const;

    //
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void textEdited();
    void clicked();
    void focusedIn();
    void focusedOut();

private:
    TextEditTweak *textEdit;
    bool textChangeIsByUser {true};
    int numberOfSpacesToReplaceTab {-1}; // negative number: won't replace TAB

    void insertSpaces(const int count);
};


//!
//! - When texts are pasted, they are inserted as plain texts.
//! - Clears the selection when focused-out.
//!
class TextEditTweak : public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEditTweak(const bool acceptEveryWheelEvent_ = false, QWidget *parent = nullptr);

signals:
    void mouseReleased();
    void focusedIn();
    void focusedOut();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void insertFromMimeData(const QMimeData *source) override;

private:
    bool acceptEveryWheelEvent;
};

#endif // TEXTEDIT_H
