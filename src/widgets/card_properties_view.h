#ifndef CARDPROPERTIESVIEW_H
#define CARDPROPERTIESVIEW_H

#include <QFrame>
#include <QTextEdit>

class CardPropertiesView : public QFrame
{
    Q_OBJECT
public:
    explicit CardPropertiesView(QWidget *parent = nullptr);

private:
    int cardId {-1};

    QTextEdit *textEdit {nullptr}; // [temp]

    void setUpWidgets();
    void setUpConnections();

    //!
    //! \param cardIdToLoad: can be -1
    //!
    void loadCard(const int cardIdToLoad);
};

#endif // CARDPROPERTIESVIEW_H
