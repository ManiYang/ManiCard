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
    explicit CustomTextEdit(QWidget *parent = nullptr);

    void clear();
    void setPlainText(const QString &text);

    void setReadOnly(const bool readonly);

    //!
    //! Sets context-menu policy for the wrapped QTextEdit.
    //!
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);

    //
    QString toPlainText() const;

signals:
    void textEdited();
    void clicked();

private:
    TextEditTweak *textEdit;
    bool textChangeIsByUser {true};
};


//!
//! - Wheel events will be accepted.
//! - Clear selection on focus-out event.
//!
class TextEditTweak : public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEditTweak(QWidget *parent = nullptr);

signals:
    void mouseReleased();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // TEXTEDIT_H
