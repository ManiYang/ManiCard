#include <limits>
#include <QDialogButtonBox>
#include <QIntValidator>
#include <QPushButton>
#include "dialog_create_relationship.h"
#include "ui_dialog_create_relationship.h"
#include "utilities/naming_rules.h"

DialogCreateRelationship::DialogCreateRelationship(
        const int cardId_, const QString &cardTitle,
        const QStringList relationshipTypesList, QWidget *parent)
            : QDialog(parent)
            , ui(new Ui::DialogCreateRelationship)
            , cardId(cardId_) {
    ui->setupUi(this);
    setWindowTitle("Create Relationship");

    setUpWidgets(cardTitle, relationshipTypesList);
    setUpConnections();

    validate();
}

DialogCreateRelationship::~DialogCreateRelationship() {
    delete ui;
}

std::optional<RelationshipId> DialogCreateRelationship::getRelationshipId() const {
    bool ok;
    const int otherCardId = ui->lineEdit->text().toInt(&ok);
    if (!ok)
        return {};

    const QString relType = ui->comboBoxRelType->currentText().trimmed();
    if (relType.isEmpty())
        return {};

    if (otherCardId == cardId) // (self loop)
        return {};

    if (ui->radioButtonFrom->isChecked())
        return RelationshipId(cardId, otherCardId, relType);
    else if (ui->radioButtonTo->isChecked())
        return RelationshipId(otherCardId, cardId, relType);
    else
        return {};
}

void DialogCreateRelationship::setUpWidgets(
        const QString &cardTitle, const QStringList &relTypesList) {
    ui->labelCardIdAndTitle->setText(QString("Card %1 (<b>%2</b>)").arg(cardId).arg(cardTitle));

    ui->lineEdit->setValidator(new QIntValidator(0, std::numeric_limits<int>::max(), this));

    ui->radioButtonFrom->setChecked(true);
    setToFromState();

    ui->comboBoxRelType->addItems(relTypesList);

    ui->labelWarningMsg->setStyleSheet("QLabel { color: red; }");
}

void DialogCreateRelationship::setUpConnections() {
    connect(ui->lineEdit, &QLineEdit::textEdited, this, [this](const QString &/*text*/) {
        validate();
    });

    //
    connect(ui->radioButtonFrom, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            setToFromState();
    });

    connect(ui->radioButtonTo, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            setToToState();
    });

    //
    connect(ui->comboBoxRelType, &QComboBox::currentTextChanged,
            this, [this](const QString &/*text*/) {
        validate();
    });
}

void DialogCreateRelationship::setToFromState() {
    ui->labelArrowFrom->setVisible(true);
    ui->labelArrowTo->setVisible(false);

    ui->labelTargetOrSource->setText("Target card ID:");
}

void DialogCreateRelationship::setToToState() {
    ui->labelArrowFrom->setVisible(false);
    ui->labelArrowTo->setVisible(true);

    ui->labelTargetOrSource->setText("Source card ID:");
}

void DialogCreateRelationship::validate() {
    ui->labelWarningMsg->setText("");

    bool otherCardIdAcceptable = true;
    {
        bool ok;
        const int otherCardId = ui->lineEdit->text().toInt(&ok);
        if (!ok) {
            otherCardIdAcceptable = false;
        }
        else {
            if (otherCardId == cardId) {
                otherCardIdAcceptable = false;
                ui->labelWarningMsg->setText("Cannot create self loop.");
            }
        }
    }

    //
    static const QRegularExpression re(regexPatternForRelationshipType);

    bool relationshipTypeAcceptable = true;
    {
        const QString relType = ui->comboBoxRelType->currentText();
        if (relType.isEmpty()) {
            relationshipTypeAcceptable = false;
        }
        else {
            if (!re.match(relType).hasMatch()) {
                relationshipTypeAcceptable = false;
                ui->labelWarningMsg->setText(
                        "Relationship type does not satisfy the naming rule.");
                        // (It's ok that this may overwrite original warning msg.)
            }
        }
    }

    //
    const bool acceptable = otherCardIdAcceptable && relationshipTypeAcceptable;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}
