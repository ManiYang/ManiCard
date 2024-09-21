#include <QDebug>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include "app_data.h"
#include "card_properties_view.h"
#include "services.h"
#include "utilities/json_util.h"
#include "utilities/naming_rules.h"
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

        buttonNewProperty = new QPushButton(QIcon(":/icons/add_black_24"), "New Property");
        layout->addWidget(buttonNewProperty, 0, Qt::AlignLeft);
        buttonNewProperty->setVisible(false);

        labelLoadingMsg = new QLabel;
        layout->addWidget(labelLoadingMsg);
        labelLoadingMsg->setVisible(false);

        customPropertiesArea.addToLayout(layout);
        customPropertiesArea.setReadonly(true);
    }

    //
    setStyleSheet(
            "QScrollBar:vertical {"
            "  width: 12px;"
            "}");

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

    buttonNewProperty->setStyleSheet(
            "QPushButton {"
            "  color: #606060;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 2px 4px 2px 2px;"
            "  background: transparent;"
            "  margin-left: 14px;"
            "}"
            "QPushButton:hover {"
            "  background: #e0e0e0;"
            "}"
            "QPushButton:pressed {"
            "  background: #c0c0c0;"
            "}");
}

void CardPropertiesView::setUpConnections() {
    //
    connect(checkBoxEdit, &QCheckBox::toggled, this, [this](bool checked) {
        customPropertiesArea.setReadonly(!checked);
        buttonNewProperty->setVisible(checked);
    });

    //
    connect(buttonNewProperty, &QCheckBox::clicked, this, [this]() {
        // ask property name
        auto *dialog = createDialogAskPropertyName(this);

        connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
            if (result != QDialog::Accepted)
                return;

            const QString propertyName = dialog->property("enteredPropertyName").toString();
            dialog->deleteLater();

            // check property name not already exists
            if (customPropertiesArea.hasPropertyName(propertyName)) {
                QMessageBox::information(
                        this, " ",
                        QString("Property \"%1\" already exists.").arg(propertyName));
                return;
            }

            //
            const QJsonValue value(QJsonValue::Null);
            customPropertiesArea.addProperty(propertyName, value);

            // call AppData
            CardPropertiesUpdate update;
            update.setCustomProperties({{propertyName, value}});

            Services::instance()->getAppData()
                    ->updateCardProperties(EventSource(this), cardId, update);

        });

        dialog->open();
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

void CardPropertiesView::onPropertyUpdated(
        const QString &propertyName, const QJsonValue &updatedValue) {
    // call AppData
    CardPropertiesUpdate update;
    update.setCustomProperties({{propertyName, updatedValue}});

    Services::instance()->getAppData()
            ->updateCardProperties(EventSource(this), cardId, update);
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

    buttonNewProperty->setVisible(false);
    labelLoadingMsg->setVisible(false);

    //
    loadCardProperties("", {});
    if (cardIdToLoad != -1) {
        labelLoadingMsg->setText("<font color=\"#888\">Loading...</font>");
        labelLoadingMsg->setVisible(true);

        Services::instance()->getAppDataReadonly()->queryCards(
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
                if (!cardData.getCustomProperties().isEmpty()) {
                    checkBoxEdit->setVisible(true);
                    checkBoxEdit->setChecked(false);
                    customPropertiesArea.setReadonly(true);
                }
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
    customPropertiesArea.clear();

    for (auto it = customProperties.constBegin(); it != customProperties.constEnd(); ++it) {
        const QString &propertyName = it.key();
        QJsonValue value = it.value();

        if (value.isUndefined()) {
            qWarning().noquote()
                    << QString("got Undefined value of property %1").arg(propertyName);
            continue;
        }

        customPropertiesArea.addProperty(propertyName, value);
    }
}

void CardPropertiesView::updateCardProperties(
        const CardPropertiesUpdate &cardPropertiesUpdate) {
    // title
    if (cardPropertiesUpdate.title.has_value())
        labelTitle->setText(cardPropertiesUpdate.title.value());

    // custom properties
    const QHash<QString, QJsonValue> customPropertiesUpdate
            = cardPropertiesUpdate.getCustomProperties();
    for (auto it = customPropertiesUpdate.constBegin();
            it != customPropertiesUpdate.constEnd(); ++it) {
        const QString &propertyName = it.key();
        const QJsonValue &updatedValue = it.value();

        if (customPropertiesArea.hasPropertyName(propertyName)) {
            if (updatedValue.isUndefined()) // (removal)
                customPropertiesArea.setProperty(propertyName, QJsonValue::Null);
            else
                customPropertiesArea.setProperty(propertyName, updatedValue);
        }
        else { // (`propertyName` does not already exists)
            if (!updatedValue.isUndefined())
                customPropertiesArea.addProperty(propertyName, updatedValue);
        }
    }
}

QDialog *CardPropertiesView::createDialogAskPropertyName(QWidget *parent) {
    auto *dialog = new QDialog(parent);

    auto *layout = new QGridLayout;
    dialog->setLayout(layout);
    {
        auto *label = new QLabel("Property Name:");
        layout->addWidget(label, 0, 0);

        auto *lineEdit = new QLineEdit;
        layout->addWidget(lineEdit, 0, 1);
        {
            lineEdit->setPlaceholderText("propertyName");
        }

        auto *labelWarningMsg = new QLabel;
        layout->addWidget(labelWarningMsg, 1, 0, 1, 2);
        {
            labelWarningMsg->setWordWrap(true);
            labelWarningMsg->setStyleSheet("color: red;");
        }

        layout->addItem(
                    new QSpacerItem(10, 10, QSizePolicy::Preferred, QSizePolicy::Expanding),
                    2, 0);

        auto *buttonBox
                = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(buttonBox, 3, 0, 1, 2);
        {
            connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

            buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }

        //
        connect(lineEdit, &QLineEdit::textEdited,
                dialog, [dialog, buttonBox, labelWarningMsg](const QString &text) {
            // validate
            static const QRegularExpression re(regexPatternForPropertyName);
            const bool validateOk = re.match(text).hasMatch();

            //
            if (validateOk) {
                dialog->setProperty("enteredPropertyName", text);
                buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
                labelWarningMsg->setText("");
            }
            else {
                buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
                labelWarningMsg->setText("Property name does not satisfy the naming rule.");
            }
        });
    }

    dialog->resize(350, 150);
    return dialog;
}

//====

CardPropertiesView::CustomPropertiesArea::CustomPropertiesArea(
        CardPropertiesView *cardPropertiesView_)
            : cardPropertiesView(cardPropertiesView_)
            , scrollArea(new QScrollArea)
            , vBoxLayout(new QVBoxLayout) {
    vBoxLayout->setContentsMargins(0, 0, 6, 0);
    vBoxLayout->setSpacing(2);
    vBoxLayout->addStretch(1);

    auto *frame = new QFrame;
    frame->setLayout(vBoxLayout);
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
    for (const PropertyWidgets &widgets: qAsConst(propertyWidgetsList)) {
        widgets.nameLabel->deleteLater();
        widgets.editor->deleteLater();
    }

    propertyWidgetsList.clear();
    addedPropertyNames.clear();

    // remove all layout items, then add a stretch
    QLayoutItem *item;
    while ((item = vBoxLayout->takeAt(0)) != nullptr)
        delete item;

    vBoxLayout->addStretch(1);
}

void CardPropertiesView::CustomPropertiesArea::addProperty(
        const QString &propertyName, const QJsonValue &value) {
    if (addedPropertyNames.contains(propertyName)) {
        Q_ASSERT(false);
        qWarning().noquote() << QString("property \"%1\" is already added").arg(propertyName);
        return;
    }

    // determine which row to insert this property (so that the properties are laid out in
    // ascending order of their names)
    int row;
    {
        auto iter = std::find_if(
                addedPropertyNames.cbegin(), addedPropertyNames.cend(),
                [propertyName](QString name) { return name > propertyName; }
        );

        if (iter == addedPropertyNames.cend())
            row = addedPropertyNames.count();
        else
            row = iter - addedPropertyNames.cbegin();
    }

    // create widgets, put them in a grid layout, and insert the grid layout to `vBoxLayout`
    // -- label
    auto *label = new QLabel(QString("%1").arg(propertyName));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setStyleSheet(
            "QLabel {"
            "  margin-top: 4px;"
            "}");

    // -- editor
    auto *editor = new PropertyValueEditor(value);
    editor->setReadonly(readonly);
    connect(editor, &PropertyValueEditor::edited,
            cardPropertiesView, [this, propertyName, editor]() {
        const QJsonValue value = editor->getValue();
        if (value.isUndefined()) // not valid
            return;

        cardPropertiesView->onPropertyUpdated(propertyName, value);
    });

    // -- grid layout
    auto gridLayout = new QGridLayout;
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setColumnMinimumWidth(0, 20);

    gridLayout->addWidget(
            label, 0, 0,
            1, 2); // row-span, column-span
    gridLayout->addWidget(editor, 1, 1);

    // --
    vBoxLayout->insertLayout(row, gridLayout);
    propertyWidgetsList.insert(row, PropertyWidgets {label, editor});
    addedPropertyNames.insert(row, propertyName);
}

void CardPropertiesView::CustomPropertiesArea::setProperty(
        const QString &propertyName, const QJsonValue &newValue) {
    const int row = addedPropertyNames.indexOf(propertyName);
    if (row == -1) {
        Q_ASSERT(false);
        qWarning().noquote() << QString("property \"%1\" does not exist").arg(propertyName);
        return;
    }

    propertyWidgetsList.at(row).editor->setValue(newValue);
}

void CardPropertiesView::CustomPropertiesArea::setReadonly(const bool readonly_) {
    readonly = readonly_;

    for (const PropertyWidgets &widgets: qAsConst(propertyWidgetsList))
        widgets.editor->setReadonly(readonly);
}

bool CardPropertiesView::CustomPropertiesArea::hasPropertyName(
        const QString &propertyName) const {
    return addedPropertyNames.contains(propertyName);
}
