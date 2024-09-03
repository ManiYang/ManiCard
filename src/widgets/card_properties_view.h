#ifndef CARDPROPERTIESVIEW_H
#define CARDPROPERTIESVIEW_H

#include <QFrame>
#include <QTextEdit>

class CardPropertiesView : public QFrame
{
    Q_OBJECT
public:
    explicit CardPropertiesView(QWidget *parent = nullptr);

    void loadCard(const int cardIdToLoad); // `cardId` can be -1

private:
    int cardId {-1};

    QTextEdit *textEdit {nullptr}; // [temp]

    void setUpWidgets();
};

#endif // CARDPROPERTIESVIEW_H
