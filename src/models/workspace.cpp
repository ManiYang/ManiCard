#include <QDebug>
#include "utilities/json_util.h"
#include "workspace.h"

QJsonObject Workspace::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"boardsOrdering", toJsonArray(boardsOrdering)},
        {"lastOpenedBoardId", lastOpenedBoardId},
        {"cardLabelToColorMapping", cardLabelToColorMapping.toJsonStr(QJsonDocument::Compact)},
        {"cardPropertiesToShow", cardPropertiesToShow.toJsonStr(QJsonDocument::Compact)}
    };
}

void Workspace::updateNodeProperties(const QJsonObject &obj) {
    if (const QJsonValue v = obj.value("name"); !v.isUndefined())
        name = v.toString();
    if (const QJsonValue v = obj.value("boardsOrdering"); !v.isUndefined())
        boardsOrdering = toIntVector(v.toArray(), -1);
    if (const QJsonValue v = obj.value("lastOpenedBoardId"); !v.isUndefined())
        lastOpenedBoardId = v.toInt();
    if (const QJsonValue v = obj.value("cardLabelToColorMapping"); v.isString()) {
        QString errorMsg;
        const auto mappingOpt = CardLabelToColorMapping::fromJsonStr(v.toString(), &errorMsg);
        if (mappingOpt.has_value()) {
            cardLabelToColorMapping = mappingOpt.value();
        }
        else {
            qWarning().noquote()
                    << QString("could not parse the string as a CardLabelToColorMapping");
            qWarning().noquote() << QString("  | string -- %1").arg(v.toString());
            qWarning().noquote() << QString("  | error msg -- %1").arg(errorMsg);
            cardLabelToColorMapping = CardLabelToColorMapping();
        }
    }
    if (const QJsonValue v = obj.value("cardPropertiesToShow"); v.isString()) {
        QString errorMsg;
        const auto dataOpt = CardPropertiesToShow::fromJsonStr(v.toString(), &errorMsg);
        if (dataOpt.has_value()) {
            cardPropertiesToShow = dataOpt.value();
        }
        else {
            qWarning().noquote()
                    << QString("could not parse the string as a CardPropertiesToShow");
            qWarning().noquote() << QString("  | string -- %1").arg(v.toString());
            qWarning().noquote() << QString("  | error msg -- %1").arg(errorMsg);
            cardPropertiesToShow = CardPropertiesToShow();
        }
    }
}

void Workspace::updateNodeProperties(const WorkspaceNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(boardsOrdering);
    UPDATE_PROPERTY(lastOpenedBoardId);
    UPDATE_PROPERTY(cardLabelToColorMapping);
    UPDATE_PROPERTY(cardPropertiesToShow);

#undef UPDATE_PROPERTY
}

QJsonObject WorkspaceNodePropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (name.has_value())
        obj.insert("name", name.value());

    if (boardsOrdering.has_value())
        obj.insert("boardsOrdering", toJsonArray(boardsOrdering.value()));

    if (lastOpenedBoardId.has_value())
        obj.insert("lastOpenedBoardId", lastOpenedBoardId.value());

    if (cardLabelToColorMapping.has_value()) {
        obj.insert(
                "cardLabelToColorMapping",
                cardLabelToColorMapping.value().toJsonStr(QJsonDocument::Compact));
    }

    if (cardPropertiesToShow.has_value()) {
        obj.insert(
                "cardPropertiesToShow",
                cardPropertiesToShow.value().toJsonStr(QJsonDocument::Compact));
    }

    return obj;
}
