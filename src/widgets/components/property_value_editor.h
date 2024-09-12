#ifndef PROPERTYVALUEEDITOR_H
#define PROPERTYVALUEEDITOR_H

#include <QComboBox>
#include <QFrame>
#include <QJsonValue>
#include <QLabel>
#include <QTextEdit>

class PropertyValueEditor : public QFrame
{
    Q_OBJECT

public:
    enum class DataType {Boolean=0, Integer, Float, String, HomogeneousList, _Count};

public:
    explicit PropertyValueEditor(
            const DataType initialDataType_, const QJsonValue &initialValue_,
            QWidget *parent = nullptr);

    void setDataTypeChangable(const bool changable); // default: true
    void setReadonly(const bool readonly_); // default: false

    DataType getDataType() const;
    QJsonValue getValue() const; // Undefined if not valid
    bool isUpdated() const; // compared with initial data-type and value

signals:
    void edited();

private:
    const DataType initialDataType;
    const QJsonValue initialValue;

    bool dataTypeChangable {true};
    bool readonly {false};

    QComboBox *comboBox {nullptr};
    QTextEdit *textEdit {nullptr};
    QLabel *labelInvalid {nullptr};

    void setUpWidgets();
    void setUpConnections();

    // tools
    static QString getDataTypeName(const DataType dataType);
    static QString getTextRepresentation(const QJsonValue &value);
};

#endif // PROPERTYVALUEEDITOR_H
