#include <QDebug>
#include <QJsonArray>
#include <QVBoxLayout>
#include "property_value_editor.h"

PropertyValueEditor::PropertyValueEditor(
        const DataType initialDataType_, const QJsonValue &initialValue_, QWidget *parent)
        : QFrame(parent)
        , initialDataType(initialDataType_)
        , initialValue(initialValue_) {
    setUpWidgets();

}

void PropertyValueEditor::setUpWidgets() {
    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    {
        comboBox = new QComboBox;
        layout->addWidget(comboBox);
        {
            for (int i = 0; i < int(DataType::_Count); ++i) {
                const auto dataTypeName = getDataTypeName(static_cast<DataType>(i));
                comboBox->addItem(dataTypeName);
            }
            comboBox->setCurrentIndex(int(initialDataType));
        }

        textEdit = new QTextEdit;
        layout->addWidget(textEdit);
        {
            textEdit->setPlainText(getTextRepresentation(initialValue));
        }

        labelInvalid = new QLabel("Invalid");
        layout->addWidget(labelInvalid);
    }

    //
    comboBox->setStyleSheet(
            "QComboBox {"
            "  font-size: 10pt;"
            "  font-weight: bold;"
            "}");
    textEdit->setStyleSheet(
            "QTextEdit {"
            "  font-size: 11pt;"
            "}");
    labelInvalid->setStyleSheet(
            "font-size: 10pt;"
            "font-weight: bold;"
            "color: red;");
}

QString PropertyValueEditor::getDataTypeName(const DataType dataType) {
    switch (dataType) {
    case DataType::Boolean: return "Boolean";
    case DataType::Integer: return "Integer";
    case DataType::Float: return "Float";
    case DataType::String: return "String";
    case DataType::HomogenouesList: return "HomogenouesList";
    case DataType::_Count: return "";
    }
    Q_ASSERT(false); // not implemented
}

QString PropertyValueEditor::getTextRepresentation(const QJsonValue &value) {
    if (value.isBool()) {
        return value.toBool() ? "true" : "false";
    }
    else if (value.isDouble()) {
        constexpr int precision = 8;
        return QString::number(value.toDouble(), 'g', precision);
    }
    else if (value.isString()) {
        return value.toString();
    }
    else if (value.isArray()) {
        const auto array = value.toArray();
        QStringList items;
        for (const QJsonValue &item: array)
            items << getTextRepresentation(item);
        return QString("[%1]").arg(items.join(", "));
    }
    else {
        qWarning().noquote() << "could not get text representation of" << value;
        return "";
    }
}
