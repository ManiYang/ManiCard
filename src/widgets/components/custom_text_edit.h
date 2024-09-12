#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>

class TextEditTweak;

//!
//! Wraps a QTextEdit.
//! Emits signal textEdited() only when the text is updated by user.
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
    //! Sets context-menu policy for the wrapped QTextEdit.
    //!
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);

    QString toPlainText() const;

    //
    QTextDocument *document() const;

signals:
    void textEdited();
    void clicked();

private:
    TextEditTweak *textEdit;
    bool textChangeIsByUser {true};
};


//!
//! Clears the selection when focused-out.
//!
class TextEditTweak : public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEditTweak(const bool acceptEveryWheelEvent_ = false, QWidget *parent = nullptr);

signals:
    void mouseReleased();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool acceptEveryWheelEvent;
};

#endif // TEXTEDIT_H
