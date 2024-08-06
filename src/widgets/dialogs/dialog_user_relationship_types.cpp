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

void DialogUserRelationshipTypes::setUpConnections() {
    connect(ui->pushButtonOk, &QPushButton::clicked, this, [this]() {
        accept();
    });
}
