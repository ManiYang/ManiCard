#include <QDebug>
#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "card_properties_view.h"
#include "services.h"

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

    // [temp]
    if (cardId == -1) {
        textEdit->clear();
    }
    else {
        auto text = QString("Card %1").arg(cardId);
        textEdit->setPlainText(text);
    }
}
