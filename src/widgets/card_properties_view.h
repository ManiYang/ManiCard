#ifndef CARDPROPERTIESVIEW_H
#define CARDPROPERTIESVIEW_H

#include <optional>
#include <QCheckBox>
#include <QFrame>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>
#include "models/card.h"

class PropertyValueEditor;

class CardPropertiesView : public QFrame
{
    Q_OBJECT
public:
    explicit CardPropertiesView(QWidget *parent = nullptr);

private:
    struct CustomPropertiesArea
    {
        explicit CustomPropertiesArea();

        void addToLayout(QBoxLayout *layout);
        void clear();
        void addProperty(const QString propertyName, const QJsonValue &value);
        void setReadonly(const bool readonly); // default: true

    private:
        QScrollArea *scrollArea;
        QGridLayout *gridLayout;

        struct PropertyData
        {
            QLabel *nameLabel;
            PropertyValueEditor *editor;
        };
        QList<PropertyData> propertiesData;

        int lastPopulatedGridRow {-1};
    };

private:
    int cardId {-1};

    QLabel *labelCardId {nullptr};
    QCheckBox *checkBoxEdit {nullptr};
    QLabel *labelTitle {nullptr};
    QLabel *labelLoadingMsg {nullptr};
    CustomPropertiesArea *customPropertiesArea {nullptr};

    void setUpWidgets();
    void setUpConnections();

    //!
    //! Won't reload if current card ID already equals `cardIdToLoad`.
    //! \param cardIdToLoad: can be -1
    //!
    void loadCard(const int cardIdToLoad);

    void loadCardProperties(
            const QString &title, const QHash<QString, QJsonValue> &customProperties);
    void updateCardProperties(const CardPropertiesUpdate &cardPropertiesUpdate);
};

#endif // CARDPROPERTIESVIEW_H
