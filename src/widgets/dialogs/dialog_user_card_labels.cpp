#include <QMessageBox>
#include <QRegularExpression>
#include "dialog_user_card_labels.h"
#include "ui_dialog_user_card_labels.h"
#include "utilities/naming_rules.h"

DialogUserCardLabels::DialogUserCardLabels(const QStringList labels, QWidget *parent)
        : QDialog(parent)
        , ui(new Ui::DialogUserCardLabels) {
    ui->setupUi(this);
    setUpConnections();

    setWindowTitle("Labels");
    ui->listWidget->addItems(labels);
}

DialogUserCardLabels::~DialogUserCardLabels() {
    delete ui;
}

QStringList DialogUserCardLabels::getLabelsList() const {
    QStringList result;
    for (int r = 0; r < ui->listWidget->count(); ++r)
        result << ui->listWidget->item(r)->text();
    return result;
}

void DialogUserCardLabels::setUpConnections() {
    static QRegularExpression re(regexPatternForCardLabelName);

    connect(ui->pushButtonAdd, &QPushButton::clicked, this, [this]() {
        QString newLabel = ui->lineEdit->text().trimmed();
        if (newLabel.isEmpty())
            return;

        // validate
        if (!re.match(newLabel).hasMatch()) {
            QMessageBox::warning(
                    this, " ", QString("\"%1\" does not satisfy the naming rules").arg(newLabel));
            return;
        }

        //
        newLabel = newLabel.toLower();
        newLabel[0] = newLabel[0].toUpper();

        // already exists?
        const auto found = ui->listWidget->findItems(
                newLabel, Qt::MatchFixedString | Qt::MatchCaseSensitive);
        if (!found.isEmpty()) {
            ui->listWidget->setCurrentItem(found.at(0));
            ui->lineEdit->clear();
            return;
        }

        //
        auto *item = new QListWidgetItem(newLabel);
        ui->listWidget->addItem(item);
        ui->listWidget->setCurrentItem(item);
        ui->lineEdit->clear();
    });

    connect(ui->pushButtonRemove, &QPushButton::clicked, this, [this]() {
        auto *item = ui->listWidget->currentItem();
        if (item != nullptr)
            delete item;
    });
}
