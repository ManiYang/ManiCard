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
#include "widgets/components/property_value_editor.h"


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

        //!
        //! \param propertyName
        //! \param dataType
        //! \param value
        //! \param readonly: if true, this property cannot be edited even if
        //!                  \e overallEditable is true
        //!
        void addProperty(
                const QString propertyName, const PropertyValueEditor::DataType dataType,
                const QJsonValue &value, const bool readonly);

        void setOverallEditable(const bool editable);

    private:
        QScrollArea *scrollArea;
        QGridLayout *gridLayout;

        struct PropertyData
        {
            QLabel *nameLabel;
            PropertyValueEditor *editor;
            bool editable;
        };
        QList<PropertyData> propertiesData;

        int lastPopulatedGridRow {-1};
        bool overallEditable {false};
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
