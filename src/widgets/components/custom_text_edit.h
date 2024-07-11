#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>

class TextEditTweak;

class CustomTextEdit : public QFrame
{
    Q_OBJECT
public:
    explicit CustomTextEdit(QWidget *parent = nullptr);

    void clear();
    void setPlainText(const QString &text);

    void setReadOnly(const bool readonly);
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);

signals:
    void textEdited(); // by user

private:
    TextEditTweak *textEdit;
    bool textChangeIsByUser {true};
};


class TextEditTweak : public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEditTweak(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
};

#endif // TEXTEDIT_H
