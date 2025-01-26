#include "card_properties_to_show.h"

CardPropertiesToShow::CardPropertiesToShow()
    : AbstractWorkspaceOrBoardSetting(SettingCategory::CardPropertiesToShow) {
}

QString CardPropertiesToShow::toJsonStr(const QJsonDocument::JsonFormat format) const {
    return ""; // [temp]
}

QString CardPropertiesToShow::schema() const {
    return QString(R"%(
    {
      "cardPropertiesToShow": [
        {
          "<propertyName>": ~"The value is $"
        },
        {
          "<propertyName>": [
            "<displayStringWhenExists>",
            "<displayStringWhenNotExists>"
          ]
        },
        ...
      ]
    }
    )%").trimmed();
}

bool CardPropertiesToShow::validate(const QString &s, QString *errorMsg) {
    return false; // [temp]
}

bool CardPropertiesToShow::setFromJsonStr(const QString &jsonStr) {
    return false; // [temp]
}
