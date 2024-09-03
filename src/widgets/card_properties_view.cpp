#include <QDebug>
#include <QVBoxLayout>
#include "card_properties_view.h"

CardPropertiesView::CardPropertiesView(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
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
