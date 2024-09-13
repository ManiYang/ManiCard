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
        explicit CustomPropertiesArea(CardPropertiesView *cardPropertiesView_);

        void addToLayout(QBoxLayout *layout);
        void clear();
        void addProperty(const QString propertyName, const QJsonValue &value);
        void setReadonly(const bool readonly_); // default: true

        bool hasPropertyName(const QString &propertyName) const;

    private:
        CardPropertiesView *const cardPropertiesView;
        QScrollArea *scrollArea;
        QVBoxLayout *vBoxLayout;

        bool readonly {true};

        struct PropertyWidgets
        {
            QLabel *nameLabel;
            PropertyValueEditor *editor;
        };
        QList<PropertyWidgets> propertyWidgetsList;
        QStringList addedPropertyNames; // in layout order
    };

private:
    int cardId {-1};

    QLabel *labelCardId {nullptr};
    QCheckBox *checkBoxEdit {nullptr};
    QLabel *labelTitle {nullptr};
    QPushButton *buttonNewProperty {nullptr};
    QLabel *labelLoadingMsg {nullptr};
    CustomPropertiesArea customPropertiesArea {this};

    void setUpWidgets();
    void setUpConnections();

    // event handlers
    void onPropertyUpdated(const QString &propertyName, const QJsonValue &updatedValue);

    //

    //!
    //! Won't reload if current card ID already equals `cardIdToLoad`.
    //! \param cardIdToLoad: can be -1
    //!
    void loadCard(const int cardIdToLoad);

    void loadCardProperties(
            const QString &title, const QHash<QString, QJsonValue> &customProperties);
    void updateCardProperties(const CardPropertiesUpdate &cardPropertiesUpdate);

    static QDialog *createDialogAskPropertyName(QWidget *parent = nullptr);
};

#endif // CARDPROPERTIESVIEW_H
