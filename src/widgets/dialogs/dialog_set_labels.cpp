#include "dialog_set_labels.h"
#include "ui_dialog_set_labels.h"
#include "utilities/lists_vectors_util.h"

#include <QMessageBox>

DialogSetLabels::DialogSetLabels(
        const QSet<QString> &initialCardLabels, const QStringList &userDefinedLabelsList,
        QWidget *parent)
            : QDialog(parent)
            , ui(new Ui::DialogSetLabels)
            , userDefinedLabelsList(userDefinedLabelsList) {
    ui->setupUi(this);
    setWindowTitle("Set Card Labels");
    setUpLabelsList();
    ui->lineEdit->setFocus();

    //
    const QVector<QString> initCardLabelsVec = sortByOrdering(
            initialCardLabels, userDefinedLabelsList, false);
    const QStringList initCardLabelsList(initCardLabelsVec.begin(), initCardLabelsVec.end());

    ui->listWidgetCurrentCardLabels->addItems(initCardLabelsList);
    ui->pushButtonRemove->setEnabled(
            ui->listWidgetCurrentCardLabels->currentItem() != nullptr);
    updateLabelsList(initCardLabelsList);

    //
    setUpConnections();
}

DialogSetLabels::~DialogSetLabels() {
    delete ui;
}

QStringList DialogSetLabels::getLabels() const {
    return getCurrentCardLabels();
}

void DialogSetLabels::setUpLabelsList() {
    for (const QString &label: userDefinedLabelsList) {
        auto *item = new QListWidgetItem(label);
        labelToLabelsListItem.insert(label, item);
        ui->listWidgetLabelsList->addItem(item);
    }
}

void DialogSetLabels::setUpConnections() {
    connect(ui->pushButtonOk, &QPushButton::clicked, this, [this]() { accept(); });

    connect(ui->pushButtonCancel, &QPushButton::clicked, this, [this]() { reject(); });

    //
    connect(ui->pushButtonRemove, &QPushButton::clicked, this, [this]() {
        auto *item = ui->listWidgetCurrentCardLabels->currentItem();
        if (item == nullptr)
            return;
        delete item;
        updateLabelsList(getCurrentCardLabels());
    });

    //
    static QRegularExpression re("^[a-zA-Z_][a-zA-Z0-9_]*$");

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

        //
        addLabelToCurrentCardLabels(newLabel);
        updateLabelsList(getCurrentCardLabels());
        ui->lineEdit->clear();
    });

    //
    connect(ui->listWidgetLabelsList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *item) {
        addLabelToCurrentCardLabels(item->text());
        updateLabelsList(getCurrentCardLabels());
    });

    //
    connect(ui->listWidgetCurrentCardLabels, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *currentItem, QListWidgetItem */*previous*/) {
        ui->pushButtonRemove->setEnabled(currentItem != nullptr);
    });
}

void DialogSetLabels::addLabelToCurrentCardLabels(const QString &label) {
    const QStringList labels = getCurrentCardLabels() << label;
    const QVector<QString> labelsVec
            = sortByOrdering(labels, userDefinedLabelsList, false);
    ui->listWidgetCurrentCardLabels->clear();
    ui->listWidgetCurrentCardLabels->addItems(QStringList(labelsVec.begin(), labelsVec.end()));

    // select the newly added label
    const auto found = ui->listWidgetCurrentCardLabels->findItems(
            label, Qt::MatchFixedString | Qt::MatchCaseSensitive);
    Q_ASSERT(!found.isEmpty());
    ui->listWidgetCurrentCardLabels->setCurrentItem(found.at(0));
}

void DialogSetLabels::updateLabelsList(const QStringList &currentCardLabels) {
    for (auto it = labelToLabelsListItem.constBegin();
            it != labelToLabelsListItem.constEnd(); ++it) {
        const QString &label = it.key();
        auto *item = it.value();

        if (currentCardLabels.contains(label))
            item->setHidden(true);
        else
            item->setHidden(false);
    }
}

QStringList DialogSetLabels::getCurrentCardLabels() const {
    QStringList labels;
    for (int i = 0; i < ui->listWidgetCurrentCardLabels->count(); ++i)
        labels << ui->listWidgetCurrentCardLabels->item(i)->text();
    return labels;
}
