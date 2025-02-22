#include <QDebug>
#include <QVBoxLayout>
#include "custom_text_browser.h"
#include "utilities/numbers_util.h"

CustomTextBrowser::CustomTextBrowser(QWidget *parent)
        : QFrame(parent)
        , textBrowser(new QTextBrowser) {
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    //
    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(textBrowser);

    textBrowser->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    connect(textBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl &link) {
        emit anchorClicked(link);
    });
}

void CustomTextBrowser::clear() {
    textBrowser->clear();
    textBrowser->setCurrentCharFormat({});
}

void CustomTextBrowser::setOpenLinks(const bool b) {
    textBrowser->setOpenLinks(b);
}

void CustomTextBrowser::setHorizontalScrollBarPolicy(const Qt::ScrollBarPolicy policy) {
    textBrowser->setHorizontalScrollBarPolicy(policy);
}

void CustomTextBrowser::setVerticalScrollBarPolicy(const Qt::ScrollBarPolicy policy) {
    textBrowser->setVerticalScrollBarPolicy(policy);
}

void CustomTextBrowser::setTextCursor(const QTextCursor &cursor) {
    textBrowser->setTextCursor(cursor);
}

void CustomTextBrowser::setPlainText(const QString &text) {
    textBrowser->setPlainText(text);
}

QTextDocument *CustomTextBrowser::document() const {
    return textBrowser->document();
}

QTextCursor CustomTextBrowser::textCursor() const {
    return textBrowser->textCursor();
}

QSize CustomTextBrowser::sizeHint() const {
    QTextDocument *doc = textBrowser->document()->clone();
    doc->setTextWidth(this->width() - 3);
    const QSize sizeHint {
        this->width(),
        nearestInteger(doc->size().height()) + 4
    };
    doc->deleteLater();
    return sizeHint;
}
