#ifndef BOARD_H
#define BOARD_H

#include <optional>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include "models/node_rect_data.h"
#include "relationship.h"

struct BoardNodePropertiesUpdate;

struct Board {
    // properties of Board node
    QString name;

    QPointF topLeftPos {0, 0}; // view's top-left corner
    double zoomRatio {1.0};

    QColor defaultNodeRectColor {170, 170, 170};
    using LabelAndColor = std::pair<QString, QColor>;
    QVector<LabelAndColor> cardLabelsAndAssociatedColors;
            // in the order of precedence (high to low)

    //
    QHash<int, NodeRectData> cardIdToNodeRectData;

    //
    QJsonObject getNodePropertiesJson() const;

    void updateNodeProperties(const QJsonObject &obj);
    void updateNodeProperties(const BoardNodePropertiesUpdate &update);
};


struct BoardNodePropertiesUpdate
{
    std::optional<QString> name;
    std::optional<QPointF> topLeftPos;
    std::optional<double> zoomRatio;
    std::optional<QColor> defaultNodeRectColor;
    std::optional<QVector<Board::LabelAndColor>> cardLabelsAndAssociatedColors;

    QJsonObject toJson() const;
    QSet<QString> keys() const;
};

#endif // BOARD_H
