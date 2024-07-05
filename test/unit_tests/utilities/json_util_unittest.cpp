#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QJsonDocument>
#include "utilities/json_util.h"

TEST(JsonUtil, JsonReader) {
    // empty object
    QJsonValue value;
    value = JsonReader(QJsonObject {}).get();
    EXPECT_EQ(value, QJsonObject());

    value = JsonReader(QJsonObject {})["a"].get();
    EXPECT_TRUE(value.isUndefined());

    value = JsonReader(QJsonObject {})[0].get();
    EXPECT_TRUE(value.isUndefined());

    // empty array
    value = JsonReader(QJsonArray {}).get();
    EXPECT_EQ(value, QJsonArray());

    value = JsonReader(QJsonArray {})["a"].get();
    EXPECT_TRUE(value.isUndefined());

    value = JsonReader(QJsonArray {})[0].get();
    EXPECT_TRUE(value.isUndefined());

    //
    const auto obj = parseAsJsonObject(R"(
        {
            "a": 1,
            "b": {
                "c": {
                    "d": 2,
                    "e": [3, 4]
                }
            },
            "f": [
                null,
                {
                    "g": 5,
                    "h": 6
                }
            ]
        }
    )");
    ASSERT_TRUE(!obj.isEmpty());

    int i = JsonReader(obj)["a"].getInt();
    EXPECT_EQ(i, 1);

    i = JsonReader(obj)["b"]["c"]["d"].getInt();
    EXPECT_EQ(i, 2);

    value = JsonReader(obj)["b"]["c"]["e"].get();
    EXPECT_EQ(value, (QJsonArray {3, 4}));

    i = JsonReader(obj)["b"]["c"]["e"][0].getInt();
    EXPECT_EQ(i, 3);

    value = JsonReader(obj)["f"][0].get();
    EXPECT_TRUE(value.isNull());

    i = JsonReader(obj)["f"][1]["h"].getInt();
    EXPECT_EQ(i, 6);

    value = JsonReader(obj)["f"][2].get();
    EXPECT_TRUE(value.isUndefined());

    value = JsonReader(obj)["a"]["x"].get();
    EXPECT_TRUE(value.isUndefined());

    value = JsonReader(obj)["a"]["x"][0]["y"].get();
    EXPECT_TRUE(value.isUndefined());

    value = JsonReader(obj)[0].get();
    EXPECT_TRUE(value.isUndefined());

    bool hasError = false;
    try {
        JsonReader(obj)["f"][2].getOrThrow();
    } catch (JsonReaderError &e) {
        hasError = true;
    }
    EXPECT_TRUE(hasError);

    hasError = false;
    try {
        JsonReader(obj)["b"]["c"]["d"].getStringOrThrow();
    } catch (JsonReaderError &e) {
        hasError = true;
    }
    EXPECT_TRUE(hasError);
}
