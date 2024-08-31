#include <QVBoxLayout>
#include "card_properties_view.h"

CardPropertiesView::CardPropertiesView(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
}

void CardPropertiesView::setText(const QString &text) {
    textEdit->setPlainText(text);
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
