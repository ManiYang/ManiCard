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
        labelCardId = new QLabel;
        layout->addWidget(labelCardId);

        labelTitle = new QLabel;
        layout->addWidget(labelTitle);
        labelTitle->setWordWrap(true);

        // [temp]
        textEdit = new QTextEdit;
        layout->addWidget(textEdit);
        textEdit->setReadOnly(true);
    }

    //
    labelCardId->setStyleSheet(
            "color: #666;"
            "font-size: 10pt;"
            "font-weight: bold;");

    labelTitle->setStyleSheet(
            "font-size: 12pt;");

    textEdit->setStyleSheet(
            "QTextEdit {"
            "  font-size: 11pt;"
            "}");
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

    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::cardPropertiesUpdated,
            this, [this](
                EventSource eventSrc, const int cardId_,
                const CardPropertiesUpdate &cardPropertiesUpdate) {
        if (eventSrc.sourceWidget == this)
            return;

        if (cardId_ != cardId)
            return;

        onCardPropertiesUpdated(cardPropertiesUpdate);
    });
}

void CardPropertiesView::loadCard(const int cardIdToLoad) {
    if (cardIdToLoad == cardId)
        return;

    cardId = cardIdToLoad;

    //
    if (cardId == -1)
        labelCardId->clear();
    else
        labelCardId->setText(QString("Card %1").arg(cardId));

    //
    if (cardIdToLoad == -1) {
        loadCardProperties("", {});
    }
    else {
        Services::instance()->getAppDataReadonly()->queryCards(
            {cardIdToLoad},
            // callback
            [this, cardIdToLoad](bool ok, const QHash<int, Card> &cardsData) {
                if (!ok || !cardsData.contains(cardIdToLoad)) {
                    textEdit->append("<font color=\"red\">Could not get card data.</font>");
                    return;
                }

                Card cardData = cardsData.value(cardIdToLoad);
                loadCardProperties(cardData.title, cardData.getCustomProperties());
            },
            this
        );
    }
}

void CardPropertiesView::loadCardProperties(
        const QString &title, const QHash<QString, QJsonValue> &customProperties) {
    labelTitle->setText(title);

    // [temp]
    if (customProperties.isEmpty()) {
        textEdit->clear();
    }
    else {
        QJsonObject customPropsJson;
        for (auto it = customProperties.constBegin(); it != customProperties.constEnd(); ++it)
            customPropsJson.insert(it.key(), it.value());
        textEdit->append(printJson(customPropsJson, false));
    }
}

void CardPropertiesView::onCardPropertiesUpdated(
        const CardPropertiesUpdate &cardPropertiesUpdate) {
    if (cardPropertiesUpdate.title.has_value()) {
        labelTitle->setText(cardPropertiesUpdate.title.value());
    }



}
