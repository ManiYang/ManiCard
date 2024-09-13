#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTimer>
#include "property_value_editor.h"
#include "utilities/json_util.h"
#include "utilities/numbers_util.h"
#include "widgets/components/custom_text_edit.h"

PropertyValueEditor::PropertyValueEditor(const QJsonValue &initialValue_, QWidget *parent)
        : QFrame(parent) {
    const auto initialValue = alteredInitialValue(initialValue_);

    setUpWidgets(initialValue);
    setUpConnections();
}

void PropertyValueEditor::setValue(const QJsonValue &value) {
    loadValue(value);
}

void PropertyValueEditor::setReadonly(const bool readonly_) {
    readonly = readonly_;

    dataTypeView.setReadonly(readonly);
    setTextEditReadonly(readonly, dataTypeView.getCurrentType());
    setStyleSheetForTextEdit(readonly, isValid);
}

QJsonValue PropertyValueEditor::getValue() const {
    return parseTextualRepresentation(
            textEdit->toPlainText(), dataTypeView.getCurrentType());
}

void PropertyValueEditor::showEvent(QShowEvent *event) {
    QFrame::showEvent(event);
    adjustTextEditHeight();
}

void PropertyValueEditor::setUpWidgets(const QJsonValue &initialValue) {
    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    {
        dataTypeView.addToLayout(layout);

        textEdit = new CustomTextEdit(false);
        layout->addWidget(textEdit);
        {
            textEdit->setFrameShape(QFrame::Box);
            textEdit->setFixedHeight(textEditMinHeight);
        }

        labelInvalid = new QLabel("Invalid");
        layout->addWidget(labelInvalid);
    }

    //
    setStyleSheetForTextEdit = [this](const bool readonly, const bool valid) {
        textEdit->setStyleSheet(
                "CustomTextEdit {"
                "  border: 1px solid " + QString(readonly ? "#ddd" : "#aaa") + ";"
                "}"
                "CustomTextEdit > QTextEdit {"
                "  font-size: 11pt;"
                "  background-color: " + (valid ? "white" : "#ffe8e8") + ";"
                "}");
    };

    labelInvalid->setStyleSheet(
            "font-size: 10pt;"
            "font-weight: bold;"
            "color: #e66;");

    // initialize
    loadValue(initialValue);
}

void PropertyValueEditor::setUpConnections() {
    connect(textEdit, &CustomTextEdit::textEdited, this, [this]() {
        QTimer::singleShot(0, this, [this]() {
            adjustTextEditHeight();
        });

        validateAndSetInvalidMsgVisible();
        setStyleSheetForTextEdit(readonly, isValid);

        if (isValid)
            emit edited();
    });
}

void PropertyValueEditor::onDataTypeSelectedByUser() {
    validateAndSetInvalidMsgVisible();
    setStyleSheetForTextEdit(readonly, isValid);

    if (isValid)
        emit edited();
}

void PropertyValueEditor::loadValue(const QJsonValue &value) {
    const DataType dataType = deduceDataType(value);
    dataTypeView.setType(dataType);
    dataTypeView.setReadonly(readonly);
    setTextEditReadonly(readonly, dataType);
    textEdit->setPlainText(getTextualRepresentation(value, dataType));

    validateAndSetInvalidMsgVisible(); // sets `isValid`
    Q_ASSERT(isValid);
    setStyleSheetForTextEdit(readonly, isValid);
}

void PropertyValueEditor::setTextEditReadonly(
        const bool overallReadonly, const DataType currentDataType) {
    const bool readonly = overallReadonly || (currentDataType == DataType::Other);
    textEdit->setReadOnly(readonly);
}

void PropertyValueEditor::adjustTextEditHeight() {
    const double documentHeight = textEdit->document()->size().height();
    int textEditHeight = ceilInteger(documentHeight) + 2;
    textEditHeight = std::max(textEditHeight, textEditMinHeight);
    textEditHeight = std::min(textEditHeight, textEditMaxHeight);
    textEdit->setFixedHeight(textEditHeight);
}

void PropertyValueEditor::validateAndSetInvalidMsgVisible() {
    const QJsonValue v = parseTextualRepresentation(
            textEdit->toPlainText(), dataTypeView.getCurrentType());
    isValid = !v.isUndefined();
    labelInvalid->setVisible(!isValid);
}

QJsonValue PropertyValueEditor::alteredInitialValue(const QJsonValue &initialValue_) {
    if (initialValue_.isUndefined()) {
        qWarning().noquote() << "got Undefined initial value, will replace it by null";
        return QJsonValue::Null;
    }
    return initialValue_;
}

QString PropertyValueEditor::getDataTypeName(const DataType dataType) {
    switch (dataType) {
    case DataType::Boolean: return "Boolean";
    case DataType::Number: return "Number";
    case DataType::String: return "String";
    case DataType::ListOfBoolean: return "List[Boolean]";
    case DataType::ListOfNumber: return "List[Number]";
    case DataType::ListOfString: return "List[String]";
    case DataType::Null: return "Null";
    case DataType::Other: return "Other Type";
    }
    Q_ASSERT(false); // not implemented
}

PropertyValueEditor::DataType PropertyValueEditor::deduceDataType(
        const QJsonValue &value, const DataType typeForEmptyArray) {
    Q_ASSERT(typeForEmptyArray == DataType::ListOfBoolean
             || typeForEmptyArray == DataType::ListOfNumber
             || typeForEmptyArray == DataType::ListOfString);

    switch (value.type()) {
    case QJsonValue::Null:
        return DataType::Null;

    case QJsonValue::Bool:
        return DataType::Boolean;

    case QJsonValue::Double:
        return DataType::Number;

    case QJsonValue::String:
        return DataType::String;

    case QJsonValue::Array:
    {
        const QJsonArray array = value.toArray();
        if (array.isEmpty())
            return typeForEmptyArray;

        QSet<QJsonValue::Type> itemTypes;
        for (const QJsonValue &item: array)
            itemTypes << item.type();
        Q_ASSERT(!itemTypes.isEmpty());

        if (itemTypes.count() > 1) // list is not homogeneous
            return DataType::Other;

        const QJsonValue::Type itemType = *itemTypes.constBegin();
        if (itemType == QJsonValue::Bool)
            return DataType::ListOfBoolean;
        else if (itemType == QJsonValue::Double)
            return DataType::ListOfNumber;
        else if(itemType == QJsonValue::String)
            return DataType::ListOfString;
        else // element is not of simple type
            return DataType::Other;
    }
    case QJsonValue::Object:
        return DataType::Other;

    case QJsonValue::Undefined:
        Q_ASSERT(false);
        return DataType::Null;
    }

    Q_ASSERT(false); // case not implemented
    return DataType::Other;
}

QString PropertyValueEditor::getTextualRepresentation(
        const QJsonValue &value, const DataType deducedDataType) {
    if (value.isUndefined()) {
        Q_ASSERT(false);
        return "";
    }

    switch (deducedDataType) {
    case DataType::Boolean: [[fallthrough]];
    case DataType::Number: [[fallthrough]];
    case DataType::ListOfBoolean: [[fallthrough]];
    case DataType::ListOfNumber: [[fallthrough]];
    case DataType::ListOfString: [[fallthrough]];
    case DataType::Null: [[fallthrough]];
    case DataType::Other:
        return stringifyJsonValue(value);

    case DataType::String:
        return value.toString();
    }

    Q_ASSERT(false); // case not implemented
    return "";
}

QJsonValue PropertyValueEditor::parseTextualRepresentation(
        const QString &text, const DataType dataType) {
    switch (dataType) {
    case DataType::String:
        return QJsonValue(text);

    case DataType::Boolean: [[fallthrough]];
    case DataType::Number: [[fallthrough]];
    case DataType::ListOfBoolean: [[fallthrough]];
    case DataType::ListOfNumber: [[fallthrough]];
    case DataType::ListOfString: [[fallthrough]];
    case DataType::Null: [[fallthrough]];
    case DataType::Other:
    {
        const QJsonValue v = parseAsJsonValue(text);
        if (v.isUndefined())
            return QJsonValue::Undefined;

        if (v == QJsonArray()) { // empty array
            if (dataType == DataType::ListOfBoolean
                    || dataType == DataType::ListOfNumber
                    || dataType == DataType::ListOfString) {
                return v;
            }
            else {
                return QJsonValue::Undefined;
            }
        }

        const auto deducedType = deduceDataType(v);
        if (deducedType != dataType)
            return QJsonValue::Undefined;
        return v;
    }
    }

    Q_ASSERT(false); // case not implemented
    return QJsonValue::Undefined;;
}

QString PropertyValueEditor::stringifyJsonValue(const QJsonValue &value) {
    if (value.isUndefined())
        return "";

    constexpr bool compact = true;
    if (value.isArray())
        return printJson(value.toArray(), compact);
    if (value.isObject())
        return printJson(value.toObject(), compact);

    auto array = QJsonArray {value};
    const QString s = printJson(array, compact);

    static const auto re = QRegularExpression(R"(^\[(.*)\]$)");
    auto m = re.match(s);
    Q_ASSERT(m.hasMatch());
    return m.captured(1);
}

QJsonValue PropertyValueEditor::parseAsJsonValue(const QString &text) {
    if (text.trimmed().isEmpty())
        return QJsonValue::Undefined;

    const auto s = QString("[%1]").arg(text);
    const auto doc = QJsonDocument::fromJson(s.toUtf8());
    if (doc.isNull())
        return QJsonValue::Undefined;

    if (!doc.isArray()) {
        Q_ASSERT(false);
        return QJsonValue::Undefined;
    }

    const auto array = doc.array();
    if (array.count() != 1) {
        Q_ASSERT(false);
        return QJsonValue::Undefined;
    }
    return array.at(0);
}

//====

PropertyValueEditor::DataTypeView::DataTypeView(PropertyValueEditor *propertyValueEditor_)
        : propertyValueEditor(propertyValueEditor_)
        , labelDataType(new QLabel)
        , comboBoxDataType(new QComboBox) {
    // populate `comboBoxDataType`
#define ADD_ITEM(item) \
    comboBoxDataType->addItem(getDataTypeName(DataType::item), int(DataType::item));

    ADD_ITEM(Boolean);
    ADD_ITEM(Number);
    ADD_ITEM(String);
    ADD_ITEM(ListOfBoolean);
    ADD_ITEM(ListOfNumber);
    ADD_ITEM(ListOfString);
    ADD_ITEM(Null);

#undef ADD_ITEM

    // (the visibility of widgets is set in addToLayout())

    //
    labelDataType->setStyleSheet(
            "color: #777;"
            "font-size: 10pt;"
            "font-weight: bold;"
            "margin: 3px 1px;"); //^>

    comboBoxDataType->setStyleSheet(
            "QComboBox {"
            "  font-size: 10pt;"
            "  color: #666;"
            "  font-weight: bold;"
            "}");

    // connections
    connect(comboBoxDataType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            propertyValueEditor, [this](int index) {
        if (!comboBoxCurrentIndexChangeIsByUser)
            return;

        bool ok;
        const int dataTypeInt = comboBoxDataType->itemData(index).toInt(&ok);
        Q_ASSERT(ok);

        currentType = static_cast<DataType>(dataTypeInt);
        labelDataType->setText(getDataTypeName(currentType));
        setActualReadonly(readonly, currentType);

        propertyValueEditor->onDataTypeSelectedByUser();
    });
}

void PropertyValueEditor::DataTypeView::addToLayout(QVBoxLayout *layout) {
    layout->addWidget(labelDataType);
    layout->addWidget(comboBoxDataType);

    setActualReadonly(readonly, currentType); // sets visibility
}

void PropertyValueEditor::DataTypeView::setType(const DataType type) {
    currentType = type;

    // set `comboBoxDataType` current item
    if (type != DataType::Other) {
        for (int i = 0; i < comboBoxDataType->count(); ++i) {
            bool ok;
            const int dataTypeInt = comboBoxDataType->itemData(i).toInt(&ok);
            Q_ASSERT(ok);

            if (dataTypeInt == int(type)) {
                comboBoxCurrentIndexChangeIsByUser = false;
                comboBoxDataType->setCurrentIndex(i);
                comboBoxCurrentIndexChangeIsByUser = true;

                break;
            }
        }
    }

    // set `labelDataType` text
    labelDataType->setText(getDataTypeName(type));

    // set actual readonly
    setActualReadonly(readonly, currentType);
}

void PropertyValueEditor::DataTypeView::setReadonly(const bool readonly_) {
    readonly = readonly_;
    setActualReadonly(readonly, currentType);
}

PropertyValueEditor::DataType PropertyValueEditor::DataTypeView::getCurrentType() const {
    return currentType;
}

void PropertyValueEditor::DataTypeView::setActualReadonly(
        const bool readonly_, const DataType dataType) {
    const bool actualReadonly = readonly_ || (dataType == DataType::Other);

    if (actualReadonly) {
        labelDataType->setVisible(true);
        comboBoxDataType->setVisible(false);
    }
    else {
        labelDataType->setVisible(false);
        comboBoxDataType->setVisible(true);
    }
}
