#include "group_box_tree.h"
#include "utilities/maps_util.h"

GroupBoxTree::GroupBoxTree() {
    nodeIdToChildItems.insert(rootId, {});
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

int GroupBoxTree::getParentOfGroupBox(const int groupBoxId) const {
    return groupBoxIdToParent.value(groupBoxId, -1);
}

int GroupBoxTree::getParentOfCard(const int cardId) const {
    return cardIdToParent.value(cardId, -1);
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

std::pair<QSet<int>, QSet<int> > GroupBoxTree::doGetAllDescendants(const int groupBoxId) const {
    // breadth-first search
    QSet<int> foundGroupBoxes;

    QSet<int> groupBoxesToVisit {groupBoxId};
    while (true) {
        QSet<int> foundInCurrentLevel;
        for (const int id: qAsConst(groupBoxesToVisit))
            foundInCurrentLevel += nodeIdToChildItems[id].childGroupBoxes;

        if (foundInCurrentLevel.isEmpty())
            break;

        foundGroupBoxes += foundInCurrentLevel;
        groupBoxesToVisit = foundGroupBoxes;
    }

    // cards
    QSet<int> cards = nodeIdToChildItems[groupBoxId].childCards;
    for (const int id: qAsConst(foundGroupBoxes))
        cards += nodeIdToChildItems[id].childCards;

    //
    return {foundGroupBoxes, cards};
}
