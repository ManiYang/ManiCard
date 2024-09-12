#include <QDebug>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include "app_data.h"
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

        labelLoadingMsg = new QLabel;
        layout->addWidget(labelLoadingMsg);
        labelLoadingMsg->setVisible(false);

        customPropertiesArea = new CustomPropertiesArea;
        customPropertiesArea->addToLayout(layout);
        customPropertiesArea->setOverallEditable(checkBoxEdit->isChecked());
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
}

void CardPropertiesView::setUpConnections() {
    //
    connect(checkBoxEdit, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked)
            customPropertiesArea->setOverallEditable(true);
        else
            customPropertiesArea->setOverallEditable(false);
    });

    // from AppData
    connect(Services::instance()->getAppData(), &AppDataReadonly::highlightedCardIdUpdated,
            this, [this](EventSource eventSrc) {
        if (eventSrc.sourceWidget == this)
            return;

        const int cardId
                = Services::instance()->getAppData()->getHighlightedCardId(); // can be -1
        loadCard(cardId);
    });

    connect(Services::instance()->getAppData(), &AppDataReadonly::cardPropertiesUpdated,
            this, [this](
                EventSource eventSrc, const int cardId_,
                const CardPropertiesUpdate &cardPropertiesUpdate) {
        if (eventSrc.sourceWidget == this)
            return;

        if (cardId_ != cardId)
            return;

        updateCardProperties(cardPropertiesUpdate);
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

    labelLoadingMsg->setVisible(false);

    //
    loadCardProperties("", {});
    if (cardIdToLoad != -1) {
        labelLoadingMsg->setText("<font color=\"#888\">Loading...</font>");
        labelLoadingMsg->setVisible(true);

        Services::instance()->getAppData()->queryCards(
            {cardIdToLoad},
            // callback
            [this, cardIdToLoad](bool ok, const QHash<int, Card> &cardsData) {
                if (!ok || !cardsData.contains(cardIdToLoad)) {
                    qWarning().noquote()
                            << QString("could not get data of card %1").arg(cardIdToLoad);
                    labelLoadingMsg->setText("<font color=\"#e77\">Failed to load data</font>");
                    return;
                }

                Card cardData = cardsData.value(cardIdToLoad);
                loadCardProperties(cardData.title, cardData.getCustomProperties());
                labelLoadingMsg->setVisible(false);
            },
            this
        );
    }
}

void CardPropertiesView::loadCardProperties(
        const QString &title, const QHash<QString, QJsonValue> &customProperties) {
    // title
    labelTitle->setText(title);

    // custom properties
    customPropertiesArea->clear();

    for (auto it = customProperties.constBegin(); it != customProperties.constEnd(); ++it) {
        const QString &propertyName = it.key();
        QJsonValue value = it.value();

        if (value.isNull() || value.isUndefined())
            continue;

        // data type
        // temp: deduce from `value`
        // todo: use schema
        bool canEdit = true;
        PropertyValueEditor::DataType dataType;

        switch (value.type()) {

        case QJsonValue::Bool:
            dataType = PropertyValueEditor::DataType::Boolean;
            break;
        case QJsonValue::Double:
            // [temp] -> float
            // todo: -> float or int, according to schema
            dataType = PropertyValueEditor::DataType::Float;
            break;
        case QJsonValue::String:
            dataType = PropertyValueEditor::DataType::String;
            break;
        case QJsonValue::Array:
        {
            // [temp] convert to readonly string
            // todo: check the list is homogeneous (of "primitive" type)
            constexpr bool compact = false;
            value = printJson(value.toArray(), compact);
            dataType = PropertyValueEditor::DataType::String;
            canEdit = false;
            break;
        }
        case QJsonValue::Object:
        {
            // convert to readonly string
            constexpr bool compact = false;
            value = printJson(value.toObject(), compact);
            dataType = PropertyValueEditor::DataType::String;
            canEdit = false;
            break;
        }
        case QJsonValue::Null: [[fallthrough]];
        case QJsonValue::Undefined:
            continue;
        }

        customPropertiesArea->addProperty(propertyName, dataType, value, !canEdit);
    }
}

void CardPropertiesView::updateCardProperties(
        const CardPropertiesUpdate &cardPropertiesUpdate) {
    // title
    if (cardPropertiesUpdate.title.has_value())
        labelTitle->setText(cardPropertiesUpdate.title.value());



}

//====

CardPropertiesView::CustomPropertiesArea::CustomPropertiesArea()
        : scrollArea(new QScrollArea)
        , gridLayout(new QGridLayout) {
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setColumnMinimumWidth(0, 20);

    auto *frame = new QFrame;
    frame->setLayout(gridLayout);
    frame->setFrameShape(QFrame::NoFrame);
    frame->setStyleSheet(
            ".QFrame {"
            "  background-color: white;"
            "}"
            "QFrame > QLabel {"
            "  font-size: 11pt;"
            "  font-weight: bold;"
            "}");

    scrollArea->setWidget(frame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setFrameShape(QFrame::NoFrame);
}

void CardPropertiesView::CustomPropertiesArea::addToLayout(QBoxLayout *layout) {
    layout->addWidget(scrollArea);
}

void CardPropertiesView::CustomPropertiesArea::clear() {
    // delete all widgets
    for (const PropertyData &data: qAsConst(propertiesData)) {
        data.nameLabel->deleteLater();
        data.editor->deleteLater();
    }
    propertiesData.clear();

    // remove all items of `gridLayout`
    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr)
        delete child;

    //
    lastPopulatedGridRow = -1;
}

void CardPropertiesView::CustomPropertiesArea::addProperty(
        const QString propertyName, const PropertyValueEditor::DataType dataType,
        const QJsonValue &value, const bool readonly) {
    // property name
    int row = ++lastPopulatedGridRow;

    auto *label = new QLabel(propertyName);
    gridLayout->addWidget(
            label, row, 0,
            1, 2 // row-span, column-span
    );

    // editor
    row = ++lastPopulatedGridRow;

    auto *editor = new PropertyValueEditor(dataType, value);
    editor->setDataTypeChangable(false);
    gridLayout->addWidget(editor, row, 1);
    editor->setReadonly(!overallEditable || readonly);

    //
    propertiesData << PropertyData {label, editor, !readonly};
}

void CardPropertiesView::CustomPropertiesArea::setOverallEditable(const bool overallEditable_) {
    overallEditable = overallEditable_;

    for (const PropertyData &data: qAsConst(propertiesData))
        data.editor->setReadonly(!overallEditable || !data.editable);
}
