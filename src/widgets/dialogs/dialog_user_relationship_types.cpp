#include <QMessageBox>
#include <QRegularExpression>
#include "dialog_user_relationship_types.h"
#include "ui_dialog_user_relationship_types.h"

DialogUserRelationshipTypes::DialogUserRelationshipTypes(const QStringList relTypes, QWidget *parent)
        : QDialog(parent)
        , ui(new Ui::DialogUserRelationshipTypes) {
    ui->setupUi(this);
    setUpConnections();

    setWindowTitle("Relationship Types");
    ui->listWidget->addItems(relTypes);
}

DialogUserRelationshipTypes::~DialogUserRelationshipTypes() {
    delete ui;
}

QStringList DialogUserRelationshipTypes::getRelationshipTypesList() const {
    QStringList relTypes;
    for (int r = 0; r < ui->listWidget->count(); ++r)
        relTypes << ui->listWidget->item(r)->text();
    return relTypes;
}

void DialogUserRelationshipTypes::setUpConnections() {
    static QRegularExpression re("^[a-zA-Z_][a-zA-Z0-9_]*$");

    connect(ui->pushButtonAdd, &QPushButton::clicked, this, [this]() {
        QString newRelType = ui->lineEdit->text().trimmed();
        if (newRelType.isEmpty())
            return;

        // validate
        if (!re.match(newRelType).hasMatch()) {
            QMessageBox::warning(
                    this, " ", QString("\"%1\" does not satisfy the naming rules").arg(newRelType));
            return;
        }

        //
        newRelType = newRelType.toUpper();

        // already exists?
        const auto found = ui->listWidget->findItems(
                newRelType, Qt::MatchFixedString | Qt::MatchCaseSensitive);
        if (!found.isEmpty()) {
            ui->listWidget->setCurrentItem(found.at(0));
            ui->lineEdit->clear();
            return;
        }

        //
        auto *item = new QListWidgetItem(newRelType);
        ui->listWidget->addItem(item);
        ui->listWidget->setCurrentItem(item);
        ui->lineEdit->clear();
    });

    //
    connect(ui->pushButtonRemove, &QPushButton::clicked, this, [this]() {
        auto *item = ui->listWidget->currentItem();
        if (item != nullptr)
            delete item;
    });
}
