#include <QDebug>
#include "group_box_tree.h"
#include "utilities/maps_util.h"

GroupBoxTree::GroupBoxTree() {
    nodeIdToChildItems.insert(rootId, {});
}

GroupBoxTree::Node GroupBoxTree::node(const int nodeId) {
    Q_ASSERT(nodeIdToChildItems.contains(nodeId));
    return Node(this, nodeId);
}

bool GroupBoxTree::set(
        const QHash<int, ChildGroupBoxesAndCards> &groupBoxIdToChildItems, QString *errorMsg) {
    clear();
    *errorMsg = "";

    //
    for (auto it = groupBoxIdToChildItems.constBegin(); it != groupBoxIdToChildItems.constEnd(); ++it) {
        const int parentId = it.key();
        const QSet<int> childGroupBoxes = it.value().first;
        const QSet<int> childCards = it.value().second;

        nodeIdToChildItems.insert(parentId, {childGroupBoxes, childCards});

        for (const int groupBoxId: childGroupBoxes) {
            if (groupBoxIdToParent.contains(groupBoxId)) {
                *errorMsg = QString("group-box %1 already has parent").arg(groupBoxId);
                return false;
            }
            groupBoxIdToParent.insert(groupBoxId, parentId);

            if (!nodeIdToChildItems.contains(groupBoxId))
                nodeIdToChildItems.insert(groupBoxId, {});
        }

        for (const int cardId: childCards) {
            if (cardIdToParent.contains(cardId)) {
                *errorMsg = QString("card %1 already has parent").arg(cardId);
                return false;
            }
            cardIdToParent.insert(cardId, parentId);
        }
    }

    // if a group-box in `nodeToChildItems.keys()` has no parent, set the root as it parent
    for (auto it = groupBoxIdToChildItems.constBegin(); it != groupBoxIdToChildItems.constEnd(); ++it) {
        if (it.key() == rootId)
            continue;

        const int groupBoxId = it.key();
        if (!groupBoxIdToParent.contains(groupBoxId)) {
            groupBoxIdToParent.insert(groupBoxId, rootId);
            nodeIdToChildItems[rootId].childGroupBoxes << groupBoxId;
        }
    }

    //
    return true;
}

void GroupBoxTree::removeGroupBox(const int groupBoxIdToRemove, const RemoveOption option) {
    const int parentId = groupBoxIdToParent.value(groupBoxIdToRemove, -1);
    if (parentId == -1)  // `groupBoxIdToRemove` not found
        return;

    nodeIdToChildItems[parentId].childGroupBoxes.remove(groupBoxIdToRemove);

    groupBoxIdToParent.remove(groupBoxIdToRemove);
    const ChildItems childItems = nodeIdToChildItems.take(groupBoxIdToRemove);

    // deal with `childItems`
    switch (option) {
    case RemoveOption::ReparentChildren:
        nodeIdToChildItems[parentId].childGroupBoxes += childItems.childGroupBoxes;
        nodeIdToChildItems[parentId].childCards += childItems.childCards;

        for (const int groupBoxId: childItems.childGroupBoxes)
            groupBoxIdToParent[groupBoxId] = parentId;
        for (const int cardId: childItems.childCards)
            cardIdToParent[cardId] = parentId;
        return;

    case RemoveOption::RemoveDescendants:
        for (const int cardId: childItems.childCards)
            cardIdToParent.remove(cardId);

        for (const int groupBoxId: childItems.childGroupBoxes) {
            const auto [descendantGroupBoxes, descendantCards] = doGetAllDescendants(groupBoxId);
            for (const int id: descendantGroupBoxes) {
                nodeIdToChildItems.remove(id);
                groupBoxIdToParent.remove(id);
            }
            for (const int id: descendantCards)
                cardIdToParent.remove(id);

            nodeIdToChildItems.remove(groupBoxId);
            groupBoxIdToParent.remove(groupBoxId);
        }
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GroupBoxTree::removeCard(const int cardIdToRemove) {
    const int parentId = cardIdToParent.value(cardIdToRemove, -1);
    if (parentId == -1) // `cardIdToRemove` not found
        return;

    nodeIdToChildItems[parentId].childCards.remove(cardIdToRemove);

    cardIdToParent.remove(cardIdToRemove);
}

void GroupBoxTree::clear() {
    nodeIdToChildItems.clear();
    groupBoxIdToParent.clear();
    cardIdToParent.clear();

    nodeIdToChildItems.insert(rootId, {});
}

int GroupBoxTree::getGroupBoxesCount() const {
    return groupBoxIdToParent.count();
}

int GroupBoxTree::getCardsCount() const {
    return cardIdToParent.count();
}

int GroupBoxTree::getParentOfGroupBox(const int groupBoxId) const {
    return groupBoxIdToParent.value(groupBoxId, -1);
}

int GroupBoxTree::getParentOfCard(const int cardId) const {
    return cardIdToParent.value(cardId, -1);
}

QSet<int> GroupBoxTree::getChildGroupBoxes(const int parentId) const{
    return nodeIdToChildItems.value(parentId).childGroupBoxes;
}

QSet<int> GroupBoxTree::getChildCards(const int parentId) const {
    return nodeIdToChildItems.value(parentId).childCards;
}

std::pair<QSet<int>, QSet<int> > GroupBoxTree::getAllDescendants(const int groupBoxId) const {
    if (!nodeIdToChildItems.contains(groupBoxId))
        return {QSet<int>{}, QSet<int>{}};
    return doGetAllDescendants(groupBoxId);
}

bool GroupBoxTree::formsSinglePath(const QSet<int> &groupBoxIds) const {
    if (groupBoxIds.isEmpty())
        return false;

    //
    bool foundParentNotWithin = false;
            // found a group-box in `groupBoxIds` whose parent in not in `groupBoxIds`?
    QSet<int> groupBoxesWithoutChildWithin = groupBoxIds;
            // will contain the group-boxes that have no child in `groupBoxIds`

    for (const int id: groupBoxIds) {
        if (!groupBoxIdToParent.contains(id)) {
            Q_ASSERT(false); // one of `groupBoxIds` does not exist
            return false;
        }

        const int parentId = groupBoxIdToParent.value(id);
        if (!groupBoxIds.contains(parentId)) {
            if (foundParentNotWithin) {
                // More than one node in `groupBoxIds` have no parent in `groupBoxIds`. That is,
                // `groupBoxIds` does not form a single tree.
                return false;
            }
            foundParentNotWithin = true;
        }
        else {
            groupBoxesWithoutChildWithin.remove(parentId);
        }
    }

    return groupBoxesWithoutChildWithin.count() <= 1;
}

void GroupBoxTree::addChildGroupBoxes(const int parentId, const QSet<int> childGroupBoxIds) {
    if (!nodeIdToChildItems.contains(parentId)) {
        Q_ASSERT(false); // `parentId` is not found
        return;
    }

    if (!(childGroupBoxIds & keySet(groupBoxIdToParent)).isEmpty()) {
        Q_ASSERT(false); // some of `childGroupBoxIds` already exist
        return;
    }

    //
    nodeIdToChildItems[parentId].childGroupBoxes += childGroupBoxIds;

    for (const int id: childGroupBoxIds) {
        groupBoxIdToParent.insert(id, parentId);
        nodeIdToChildItems.insert(id, {});
    }
}

void GroupBoxTree::addChildCards(const int parentId, const QSet<int> childCardIds) {
    if (!nodeIdToChildItems.contains(parentId)) {
        Q_ASSERT(false); // `parentId` is not found
        return;
    }

    if (!(childCardIds & keySet(cardIdToParent)).isEmpty()) {
        Q_ASSERT(false); // some of `childCardIds` already exist
        return;
    }

    //
    nodeIdToChildItems[parentId].childCards += childCardIds;

    for (const int id: childCardIds)
        cardIdToParent.insert(id, parentId);
}

std::pair<QSet<int>, QSet<int> > GroupBoxTree::doGetAllDescendants(const int groupBoxId) const {
    // breadth-first search
    QSet<int> groupBoxesResult;

    QSet<int> groupBoxesToVisit {groupBoxId};
    while (true) {
        QSet<int> foundInCurrentLevel;
        for (const int id: qAsConst(groupBoxesToVisit))
            foundInCurrentLevel += nodeIdToChildItems[id].childGroupBoxes;

        if (foundInCurrentLevel.isEmpty())
            break;

        groupBoxesResult += foundInCurrentLevel;
        groupBoxesToVisit = foundInCurrentLevel;
    }

    // cards
    QSet<int> cardsResult = nodeIdToChildItems[groupBoxId].childCards;
    for (const int id: qAsConst(groupBoxesResult))
        cardsResult += nodeIdToChildItems[id].childCards;

    //
    return {groupBoxesResult, cardsResult};
}

//======

void GroupBoxTree::Node::addChildGroupBoxes(const QSet<int> childGroupBoxIds) {
    tree->addChildGroupBoxes(nodeId, childGroupBoxIds);
}

void GroupBoxTree::Node::addChildCards(const QSet<int> childCardIds) {
    tree->addChildCards(nodeId, childCardIds);
}
