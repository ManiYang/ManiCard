#include <QDebug>
#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "card_properties_view.h"
#include "services.h"
#include "utilities/json_util.h"

CardPropertiesView::CardPropertiesView(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpConnections();
}

void CardPropertiesView::setUpWidgets() {
    setFrameShape(QFrame::NoFrame);

    //
    auto *layout = new QVBoxLayout;
    setLayout(layout);
    {
        // [temp]
        textEdit = new QTextEdit;
        layout->addWidget(textEdit);
        textEdit->setReadOnly(true);
    }
}

void CardPropertiesView::setUpConnections() {
    // from AppData
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::highlightedCardIdUpdated,
            this, [this](EventSource eventSrc) {
        if (eventSrc.sourceWidget == this)
            return;

        const int cardId
                = Services::instance()->getAppDataReadonly()->getHighlightedCardId(); // can be -1
        loadCard(cardId);
    });
}

void CardPropertiesView::loadCard(const int cardIdToLoad) {
    if (cardIdToLoad == cardId)
        return;

    cardId = cardIdToLoad;

    textEdit->clear();
    if (cardIdToLoad == -1)
        return;

    //
    Services::instance()->getAppDataReadonly()->queryCards(
        {cardIdToLoad},
        // callback
        [this, cardIdToLoad](bool ok, const QHash<int, Card> &cardsData) {
            if (!ok || !cardsData.contains(cardIdToLoad)) {
                textEdit->append("<font color=\"red\">Could not get card data.</font>");
                return;
            }

            Card cardData = cardsData.value(cardIdToLoad);

            textEdit->append(QString("<b>%1</b>").arg(cardData.title));

            QJsonObject customProperties;
            {
                const QJsonObject propertiesJson = cardData.getPropertiesJson();
                const auto customPropertyNames = cardData.getCustomPropertyNames();
                for (const QString &name: customPropertyNames)
                    customProperties.insert(name, propertiesJson.value(name));
            }
            textEdit->append(printJson(customProperties, false));


        },
        this
    );

    auto text = QString("Card %1").arg(cardId);
    textEdit->append(text);





}
