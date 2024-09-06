#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "card_properties_view.h"
#include "services.h"
#include "utilities/json_util.h"
#include "widgets/components/property_value_editor.h"

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
        auto *hlayout = new QHBoxLayout;
        layout->addLayout(hlayout);
        {
            labelCardId = new QLabel;
            hlayout->addWidget(labelCardId);

            hlayout->addStretch();

            checkBoxEdit = new QCheckBox("Edit");
            hlayout->addWidget(checkBoxEdit);
            checkBoxEdit->setChecked(false);
            checkBoxEdit->setVisible(false);
        }

        labelTitle = new QLabel;
        layout->addWidget(labelTitle);
        labelTitle->setWordWrap(true);

//        // [temp]
//        textEdit = new QTextEdit;
//        layout->addWidget(textEdit);
//        textEdit->setReadOnly(true);


        layout->addStretch();
    }

    //
    labelCardId->setStyleSheet(
            "color: #444;"
            "font-size: 11pt;"
            "font-weight: bold;");

    checkBoxEdit->setStyleSheet(
            "color: #444;"
            "font-size: 11pt;"
            "font-weight: bold;");

    labelTitle->setStyleSheet(
            "font-size: 13pt;"
            "font-weight: bold;");

//    textEdit->setStyleSheet(
//            "QTextEdit {"
//            "  font-size: 11pt;"
//            "}");
}

void CardPropertiesView::setUpConnections() {
    //
    connect(checkBoxEdit, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked)
            qDebug() << "set editable...";
        else
            qDebug() << "set readonly...";
        // todo ...


    });

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
    if (cardId == -1) {
        labelCardId->clear();
        checkBoxEdit->setVisible(false);
    }
    else {
        labelCardId->setText(QString("Card %1").arg(cardId));
        checkBoxEdit->setVisible(true);
        checkBoxEdit->setChecked(false);
    }

    //
    loadCardProperties("", {});
    if (cardIdToLoad != -1) {
        Services::instance()->getAppDataReadonly()->queryCards(
            {cardIdToLoad},
            // callback
            [this, cardIdToLoad](bool ok, const QHash<int, Card> &cardsData) {
                if (!ok || !cardsData.contains(cardIdToLoad)) {
                    qWarning().noquote()
                            << QString("could not get data of card %1").arg(cardIdToLoad);
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

//    // [temp]
//    if (customProperties.isEmpty()) {
//        textEdit->clear();
//    }
//    else {
//        QJsonObject customPropsJson;
//        for (auto it = customProperties.constBegin(); it != customProperties.constEnd(); ++it)
//            customPropsJson.insert(it.key(), it.value());
//        textEdit->append(printJson(customPropsJson, false));
//    }
}

void CardPropertiesView::onCardPropertiesUpdated(
        const CardPropertiesUpdate &cardPropertiesUpdate) {
    if (cardPropertiesUpdate.title.has_value()) {
        labelTitle->setText(cardPropertiesUpdate.title.value());
    }



}
