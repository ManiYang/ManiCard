#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QColor>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>

struct WorkspaceNodePropertiesUpdate;

struct Workspace {
    Workspace() : defaultNodeRectColor(defaultNodeRectColorFallback) {}

    // properties of `Workspace` node
    QString name;
    QVector<int> boardsOrdering;
    int lastOpenedBoardId {-1};

    QColor defaultNodeRectColor;
    using LabelAndColor = std::pair<QString, QColor>;
    QVector<LabelAndColor> cardLabelsAndAssociatedColors; // in the order of precedence (high to low)

    //
    QSet<int> boardIds;

    //
    QJsonObject getNodePropertiesJson() const;

    void updateNodeProperties(const QJsonObject &obj);
    void updateNodeProperties(const WorkspaceNodePropertiesUpdate &update);

private:
    inline static const QColor defaultNodeRectColorFallback {170, 170, 170};
};


struct WorkspaceNodePropertiesUpdate {
    std::optional<QString> name;
    std::optional<QVector<int>> boardsOrdering;
    std::optional<int> lastOpenedBoardId;
    std::optional<QColor> defaultNodeRectColor;
    std::optional<QVector<Workspace::LabelAndColor>> cardLabelsAndAssociatedColors;

    QJsonObject toJson() const;
};

#endif // WORKSPACE_H
