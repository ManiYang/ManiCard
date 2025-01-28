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
    explicit CustomTextEdit(QWidget *parent = nullptr);

    void clear(const bool resetFormat);

    void setPlainText(const QString &text);
    void setMarkdown(const QString &text);

    void setReadOnly(const bool readonly);
    void enableSetEveryWheelEventAccepted(const bool enable);
    void obtainFocus();
    void setVerticalScrollBarTurnedOn(const bool b);

    void setLineHeightPercent(const int percentage);
    void setParagraphSpacing(const double spacing);

    void setTextCursorPosition(const int pos);

    //!
    //! \param numberOfSpaces: if < 0, won't replace TAB
    //!
    void setReplaceTabBySpaces(const int numberOfSpaces);

    //!
    //! Sets context-menu policy for the wrapped QTextEdit.
    //!
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);

    //

    QString toPlainText() const;

    QTextDocument *document() const;

    bool isVerticalScrollBarVisible() const;

    int currentTextCursorPosition() const;

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
    explicit TextEditTweak(QWidget *parent = nullptr);

    void enableSetEveryWheelEventAccepted(const bool enable);

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
    bool setEveryWheelEventAccepted {false};
};

#endif // TEXTEDIT_H
