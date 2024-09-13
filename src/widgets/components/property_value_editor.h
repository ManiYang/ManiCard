#ifndef PROPERTYVALUEEDITOR_H
#define PROPERTYVALUEEDITOR_H

#include <QComboBox>
#include <QFrame>
#include <QJsonValue>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

class CustomTextEdit;

class PropertyValueEditor : public QFrame
{
    Q_OBJECT

public:
    //!
    //! \param initialValue_: If is Undefined, will be replaced by \c null. The (initial) data
    //!         type will be deduced from it. If the data type is one that the class cannot
    //!         perform validation on, the whole editor will be readonly (even after
    //!         \c setReadonly(false) is called).
    //! \param parent
    //!
    explicit PropertyValueEditor(
            const QJsonValue &initialValue_, QWidget *parent = nullptr);

    void setValue(const QJsonValue &value);

    //!
    //! If the value is of a data type that the class cannot perform validation on, the whole
    //! editor is readonly even if parameter \e readonly_ is false.
    //!
    void setReadonly(const bool readonly_); // default: false

    QJsonValue getValue() const; // Undefined if not valid

signals:
    void edited(); // emitted only when the value is valid

protected:
    void showEvent(QShowEvent *event) override;

private:
    // data types for validation of edited value:
    enum class DataType {
        Boolean, Number, String,
        ListOfBoolean, ListOfNumber, ListOfString,
        Null,
        Other, // This represents all other data types, which the class doesn't know how to
               // perform validation on. The whole editor is read-only if it has this type
               // (overriding the member variable `readonly`).
    };

    struct DataTypeView
    {
        explicit DataTypeView(PropertyValueEditor *propertyValueEditor_);
        void addToLayout(QVBoxLayout *layout);
        void setType(const DataType type);

        //!
        //! If current data type is Other, `this` will be readonly even if parameter `readonly`
        //! is false.
        //!
        void setReadonly(const bool readonly);

        DataType getCurrentType() const;

    private:
        PropertyValueEditor *const propertyValueEditor;
        QLabel *labelDataType;
        QComboBox *comboBoxDataType;
        bool comboBoxCurrentIndexChangeIsByUser {true};

        DataType currentType {DataType::Other};
        bool readonly {false};

        void setActualReadonly(const bool readonly_, const DataType dataType);
    };

private:
    const int textEditMinHeight {24};
    const int textEditMaxHeight {72};

    CustomTextEdit *textEdit {nullptr};
    QLabel *labelInvalid {nullptr};
    DataTypeView dataTypeView {this};

    bool readonly {false};
    bool isValid {true};

    std::function<void (const bool readonly, const bool valid)> setStyleSheetForTextEdit;

    void setUpWidgets(const QJsonValue &initialValue);
    void setUpConnections();

    // event handler
    void onDataTypeSelectedByUser();

    //
    void loadValue(const QJsonValue &value);
    void setTextEditReadonly(const bool overallReadonly, const DataType currentDataType);
    void adjustTextEditHeight();
    void validateAndSetInvalidMsgVisible(); // sets `isValid`

    // tools
    static QJsonValue alteredInitialValue(const QJsonValue &initialValue_);

    static QString getDataTypeName(const DataType dataType);

    static DataType deduceDataType(
            const QJsonValue &value, const DataType typeForEmptyArray = DataType::ListOfString);

    //!
    //! Determine how `value` is represented as a string in `textEdit`
    //!
    static QString getTextualRepresentation(
            const QJsonValue &value, const DataType deducedDataType);

    //!
    //! \return Undefined if failed to parse `text` as `dataType`
    //!
    static QJsonValue parseTextualRepresentation(const QString &text, const DataType dataType);

    static QString stringifyJsonValue(const QJsonValue &value);

    //!
    //! \return Undefined if failed (including the case where `text` is empty)
    //!
    static QJsonValue parseAsJsonValue(const QString &text);
};

#endif // PROPERTYVALUEEDITOR_H
