#ifndef CARDPROPERTIESVIEW_H
#define CARDPROPERTIESVIEW_H

#include <QFrame>
#include <QTextEdit>

class CardPropertiesView : public QFrame
{
    Q_OBJECT
public:
    explicit CardPropertiesView(QWidget *parent = nullptr);

    void setText(const QString &text); // [temp]

private:
    QTextEdit *textEdit {nullptr}; // [temp]

    void setUpWidgets();
};

#endif // CARDPROPERTIESVIEW_H
