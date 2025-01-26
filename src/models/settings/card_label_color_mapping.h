#ifndef CARD_LABEL_COLOR_MAPPING_H
#define CARD_LABEL_COLOR_MAPPING_H

#include <QColor>
#include <QVector>
#include "models/settings/abstract_setting.h"

struct CardLabelToColorMapping : public AbstractWorkspaceOrBoardSetting
{
    explicit CardLabelToColorMapping();

    using LabelAndColor = std::pair<QString, QColor>;
    QVector<LabelAndColor> cardLabelsAndAssociatedColors; // in the order of precedence (high to low)
    QColor defaultNodeRectColor {defaultNodeRectColorFallback};

    //
    QString toJsonStr(const QJsonDocument::JsonFormat format) const override;
    QString schema() const override;
    bool validate(const QString &s, QString *errorMsg = nullptr) override;
    bool setFromJsonStr(const QString &jsonStr) override;

    //!
    //! \return \c nullopt if failed
    //!
    static std::optional<CardLabelToColorMapping> fromJsonStr(
            const QString &jsonStr, QString *errorMsg = nullptr);

    CardLabelToColorMapping &operator = (const CardLabelToColorMapping &other);

private:
    inline static const QColor defaultNodeRectColorFallback {170, 170, 170};
};

#endif // CARD_LABEL_COLOR_MAPPING_H
