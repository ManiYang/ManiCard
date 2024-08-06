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
    connect(ui->pushButtonAdd, &QPushButton::clicked, this, [this]() {
        const QString newRelType = ui->lineEdit->text().trimmed().toUpper();
        if (newRelType.isEmpty())
            return;

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
