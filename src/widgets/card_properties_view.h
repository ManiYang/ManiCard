#ifndef CARDPROPERTIESVIEW_H
#define CARDPROPERTIESVIEW_H

#include <optional>
#include <QCheckBox>
#include <QFrame>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include "models/card.h"

class CardPropertiesView : public QFrame
{
    Q_OBJECT
public:
    explicit CardPropertiesView(QWidget *parent = nullptr);

private:
    int cardId {-1};

    QLabel *labelCardId {nullptr};
    QCheckBox *checkBoxEdit {nullptr};
    QLabel *labelTitle {nullptr};
//    QTextEdit *textEdit {nullptr}; // [temp]

    void setUpWidgets();
    void setUpConnections();

    //!
    //! Won't reload if current card ID is already `cardIdToLoad`.
    //! \param cardIdToLoad: can be -1
    //!
    void loadCard(const int cardIdToLoad);

    void loadCardProperties(
            const QString &title, const QHash<QString, QJsonValue> &customProperties);
    void onCardPropertiesUpdated(const CardPropertiesUpdate &cardPropertiesUpdate);
};

#endif // CARDPROPERTIESVIEW_H
