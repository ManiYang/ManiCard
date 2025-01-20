#include <QAbstractItemModel>
#include <QColorDialog>
#include <QFont>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QTableWidgetItem>
#include "app_data_readonly.h"
#include "dialog_workspace_card_colors.h"
#include "services.h"
#include "ui_dialog_workspace_card_colors.h"
#include "utilities/naming_rules.h"

namespace {
enum class Column {Label=0, Color, Precedence};
QPixmap drawColorSamplePixmap(const QColor &color);
}

DialogWorkspaceCardColors::DialogWorkspaceCardColors(
        const QString &workspaceName,
        const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
        const QColor &defaultNodeRectColor, QWidget *parent)
            : QDialog(parent)
            , ui(new Ui::DialogWorkspaceCardColors) {
    ui->setupUi(this);
    setWindowTitle("Card Colors");

    setUpWidgets(workspaceName, cardLabelsAndAssociatedColors, defaultNodeRectColor);
    setUpConnections();

    validateLabels();
}

DialogWorkspaceCardColors::~DialogWorkspaceCardColors() {
    delete ui;
}

QVector<DialogWorkspaceCardColors::LabelAndColor>
DialogWorkspaceCardColors::getCardLabelsAndAssociatedColors() const {
    QVector<LabelAndColor> result;
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        const QString label = ui->tableWidget->item(row, int(Column::Label))->text();

        ColorDisplayWidget *colorDisplayWidget = qobject_cast<ColorDisplayWidget *>(
                ui->tableWidget->cellWidget(row, int(Column::Color)));
        Q_ASSERT(colorDisplayWidget != nullptr);
        const QColor color = colorDisplayWidget->getColor();

        result << std::make_pair(label, color);
    }
    return result;
}

QColor DialogWorkspaceCardColors::getDefaultColor() const {
    return QColor(ui->labelDefaultColorHex->text());
}

void DialogWorkspaceCardColors::setUpWidgets(
        const QString &workspaceName,
        const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
        const QColor &defaultNodeRectColor) {
    // title
    ui->labelTitle->setText(QString("Workspace: <b>%1</b>").arg(workspaceName));
    {
        QFont f = font();
        f.setPointSize(12);
        ui->labelTitle->setFont(f);
    }

    // label-color association table
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels({"Card Label", "Color", "Precedence"});
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    for (int i = 0; i < cardLabelsAndAssociatedColors.count(); ++i) {
        auto &labelAndColor = cardLabelsAndAssociatedColors.at(i);
        addRowToLabelColorAssociationTable(labelAndColor.first, labelAndColor.second);
    }
    updatePrecedenceNumbers();

    //
    ui->labelWarningMsg->setVisible(false);
    ui->labelWarningMsg->setWordWrap(true);

    // default color
    setDefaultColor(defaultNodeRectColor);

    // buttons
    ui->buttonUp->setToolTip("Raise precedence");
    buttonToIcon.insert(ui->buttonUp, Icon::ArrowNorth);
    ui->buttonUp->setIconSize({24, 24});

    ui->buttonDown->setToolTip("Lower precedence");
    buttonToIcon.insert(ui->buttonDown, Icon::ArrowSouth);
    ui->buttonDown->setIconSize({24, 24});

    ui->buttonPickColor->setEnabled(false);
    ui->buttonRemove->setEnabled(false);
    ui->buttonUp->setEnabled(false);
    ui->buttonDown->setEnabled(false);

    // button icons
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = buttonToIcon.constBegin(); it != buttonToIcon.constEnd(); ++it)
        it.key()->setIcon(Icons::getIcon(it.value(), theme));

    //
    ui->labelWarningMsg->setStyleSheet("color: red;");

    ui->tableWidget->setStyleSheet(
            "QHeaderView::section {"
            "  font-weight: bold;"
            "}");
}

void DialogWorkspaceCardColors::setUpConnections() {
    // ui->tableWidget
    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, [this]() {
        if (ui->tableWidget->selectedItems().count() > 0) {
            ui->buttonPickColor->setEnabled(true);
            ui->buttonRemove->setEnabled(true);
            ui->buttonUp->setEnabled(true);
            ui->buttonDown->setEnabled(true);
        }
        else {
            ui->buttonPickColor->setEnabled(false);
            ui->buttonRemove->setEnabled(false);
            ui->buttonUp->setEnabled(false);
            ui->buttonDown->setEnabled(false);
        }
    });

    connect(ui->tableWidget->itemDelegate(), &QAbstractItemDelegate::commitData,
            this, [this](QWidget */*editor*/) {
        // (cell editing finished)
        validateLabels();
    });

    // buttons
    connect(ui->buttonAdd, &QPushButton::clicked, this, [this]() {
        addRowToLabelColorAssociationTable("Enter Label", QColor(170, 170, 170));
        updatePrecedenceNumbers();

        const int addedRow = ui->tableWidget->rowCount() - 1;
        ui->tableWidget->editItem(ui->tableWidget->item(addedRow, int(Column::Label)));
        validateLabels();
    });

    connect(ui->buttonPickColor, &QPushButton::clicked, this, [this]() {
        if (ui->tableWidget->selectedItems().isEmpty())
            return;
        const int row = ui->tableWidget->row(ui->tableWidget->selectedItems().first());
        onUserToPickColorForRow(row);
    });

    connect(ui->buttonRemove, &QPushButton::clicked, this, [this]() {
        if (ui->tableWidget->selectedItems().isEmpty())
            return;
        const int row = ui->tableWidget->row(ui->tableWidget->selectedItems().first());

        const QString label = ui->tableWidget->item(row, int(Column::Label))->text();
        onUserToRemoveRow(row, label);
    });

    connect(ui->buttonEditDefaultColor, &QPushButton::clicked, this, [this]() {
        const QColor newColor = QColorDialog::getColor(
                QColor(ui->labelDefaultColorHex->text()), this, "Select Default Color");
        if (!newColor.isValid())
            return;
        setDefaultColor(newColor);
    });

    connect(ui->buttonUp, &QPushButton::clicked, this, [this]() {
        if (ui->tableWidget->selectedItems().isEmpty())
            return;
        const int row = ui->tableWidget->row(ui->tableWidget->selectedItems().first());
        if (row == 0)
            return;

        swapRows(row, row - 1);
        ui->tableWidget->selectRow(row - 1); // precedence cells are not swapped
    });

    connect(ui->buttonDown, &QPushButton::clicked, this, [this]() {
        if (ui->tableWidget->selectedItems().isEmpty())
            return;
        const int row = ui->tableWidget->row(ui->tableWidget->selectedItems().first());
        if (row == (ui->tableWidget->rowCount() - 1))
            return;

        swapRows(row, row + 1);
        ui->tableWidget->selectRow(row + 1); // precedence cells are not swapped
    });
}

void DialogWorkspaceCardColors::onUserToPickColorForRow(const int row) {
    QWidget *w = ui->tableWidget->cellWidget(row, int(Column::Color));
    ColorDisplayWidget *colorDisplayWidget = qobject_cast<ColorDisplayWidget *>(w);
    if (colorDisplayWidget == nullptr) {
        Q_ASSERT(false);
        return;
    }

    // get label and color of the row
    const QString label = ui->tableWidget->item(row, int(Column::Label))->text();

    const QColor color = colorDisplayWidget->getColor();
    if (!color.isValid()) {
        Q_ASSERT(false);
        return;
    }

    // show color dialog
    const QColor newColor = QColorDialog::getColor(
            color, this, QString("Select color for card label %1").arg(label));
    if (!newColor.isValid())
        return;

    //
    colorDisplayWidget->setColor(newColor);
}

void DialogWorkspaceCardColors::onUserToRemoveRow(const int row, const QString &cardLabel) {
    const auto r = QMessageBox::question(
            this, " ",
            QString("Remove the color association for card label <b>%1</b>?").arg(cardLabel));
    if (r != QMessageBox::Yes)
        return;

    //
    ui->tableWidget->removeRow(row);
    updatePrecedenceNumbers();
    validateLabels();
}

void DialogWorkspaceCardColors::addRowToLabelColorAssociationTable(
        const QString &label, const QColor &color) {
    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row + 1);

    // label
    {
        auto *item = new QTableWidgetItem(label);
        ui->tableWidget->setItem(row, int(Column::Label), item);
    }

    // color
    {
        auto *colorDisplayWidget = new ColorDisplayWidget;
        colorDisplayWidget->setColor(color);

        ui->tableWidget->setCellWidget(row, int(Column::Color), colorDisplayWidget);

        connect(colorDisplayWidget, &ColorDisplayWidget::doubleClicked,
                this, [this, colorDisplayWidget]() {
            const int row = getRowOfColorDisplayWidget(colorDisplayWidget);
            if (row == -1)
                return;
            onUserToPickColorForRow(row);
        });
    }

    // precedence
    {
        auto *item = new QTableWidgetItem;
        item->setFlags(item->flags() & ~Qt::ItemIsEditable); // set not editable
        ui->tableWidget->setItem(row, int(Column::Precedence), item);
    }
}

void DialogWorkspaceCardColors::swapRows(const int row1, const int row2) {
    Q_ASSERT(row1 >= 0 && row1 < ui->tableWidget->rowCount());
    Q_ASSERT(row2 >= 0 && row2 < ui->tableWidget->rowCount());

    if (row1 == row2)
        return;

    // label
    {
        QTableWidgetItem *item1 = ui->tableWidget->takeItem(row1, int(Column::Label));
        QTableWidgetItem *item2 = ui->tableWidget->takeItem(row2, int(Column::Label));

        ui->tableWidget->setItem(row1, int(Column::Label), item2);
        ui->tableWidget->setItem(row2, int(Column::Label), item1);
    }

    // color
    {
        ColorDisplayWidget *colorDisplayWidget1 = qobject_cast<ColorDisplayWidget *>(
                ui->tableWidget->cellWidget(row1, int(Column::Color)));
        ColorDisplayWidget *colorDisplayWidget2 = qobject_cast<ColorDisplayWidget *>(
                ui->tableWidget->cellWidget(row2, int(Column::Color)));
        Q_ASSERT(colorDisplayWidget1 != nullptr);
        Q_ASSERT(colorDisplayWidget2 != nullptr);

        const QColor color1 = colorDisplayWidget1->getColor();
        const QColor color2 = colorDisplayWidget2->getColor();

        colorDisplayWidget1->setColor(color2);
        colorDisplayWidget2->setColor(color1);
    }

    // (precedence cells remains unchanged)
}

void DialogWorkspaceCardColors::updatePrecedenceNumbers() {
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        const QString text = (row == 0) ? "1 (highest)" : QString::number(row + 1);
        ui->tableWidget->item(row, int(Column::Precedence))->setText(text);
    }
}

void DialogWorkspaceCardColors::validateLabels() {
    static const QRegularExpression re(regexPatternForCardLabelName);

    // check each label is valid
    QStringList allLabels;
    QString errorMsg;
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        const QString label = ui->tableWidget->item(row, int(Column::Label))->text().trimmed();

        if (label.isEmpty()) {
            errorMsg = "Label cannot be empty.";
            break;
        }

        if (label == "Card") {
            errorMsg = "Label cannot be \"Card\".";
            break;
        }

        if (!re.match(label).hasMatch()) {
            errorMsg = QString("Label \"%1\" does not satisfy the naming rule.").arg(label);
            break;
        }

        allLabels << label;
    }

    // check no duplicated labels
    if (errorMsg.isEmpty()) {
        const QSet<QString> set(allLabels.constBegin(), allLabels.constEnd());
        if (allLabels.count() != set.count())
            errorMsg = "There is duplicated label.";
    }

    //
    if (!errorMsg.isEmpty()) {
        ui->labelWarningMsg->setText(errorMsg);
        ui->labelWarningMsg->setVisible(true);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
        ui->labelWarningMsg->setVisible(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void DialogWorkspaceCardColors::setDefaultColor(const QColor &color) {
    ui->labelDefaultColor->setPixmap(drawColorSamplePixmap(color));
    ui->labelDefaultColorHex->setText(color.name());
}

int DialogWorkspaceCardColors::getRowOfColorDisplayWidget(
        ColorDisplayWidget *colorDisplayWidget) const {
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        QWidget *w = ui->tableWidget->cellWidget(r, int(Column::Color));
        if (w == colorDisplayWidget)
            return r;
    }
    return -1;
}

//====

ColorDisplayWidget::ColorDisplayWidget(QWidget *parent)
        : QWidget(parent) {
    auto *layout = new QHBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(2, 2, 2, 2);
    {
        labelColorSample = new QLabel;
        layout->addWidget(labelColorSample);
    }
    {
        labelColorHex = new QLabel;
        layout->addWidget(labelColorHex);
    }
    layout->addStretch();
}

void ColorDisplayWidget::setColor(const QColor &color) {
    labelColorSample->setPixmap(drawColorSamplePixmap(color));
    labelColorHex->setText(color.name(QColor::HexRgb));
}

QColor ColorDisplayWidget::getColor() const {
    return QColor(labelColorHex->text());
}

void ColorDisplayWidget::mouseDoubleClickEvent(QMouseEvent */*event*/) {
    emit doubleClicked();
}

//====

namespace {
QPixmap drawColorSamplePixmap(const QColor &color) {
    const QSize size(26, 26);

    QPixmap pixmap(size);
    QPainter painter(&pixmap);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(color));
    painter.drawRect(QRect({0, 0}, size));

    painter.setPen(QPen(QColor(100, 100, 100), 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(1, 1, size.width()-2, size.height()-2);

    return pixmap;
}
}
