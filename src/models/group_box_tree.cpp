#include <QDebug>
#include "group_box_tree.h"
#include "utilities/maps_util.h"

GroupBoxTree::GroupBoxTree() {
    nodeIdToChildItems.insert(rootId, {});
}

GroupBoxTree::ContainerNode GroupBoxTree::containerNode(const int nodeId) {
    Q_ASSERT(nodeIdToChildItems.contains(nodeId));
    return ContainerNode(this, nodeId);
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

        if (parentId == rootId) {
            if (!childCards.isEmpty()) {
                *errorMsg = "root node (the Board) cannot have child cards.";
                return false;
            }
            nodeIdToChildItems.insert(parentId, {childGroupBoxes, QSet<int> {}});
        }
        else {
            nodeIdToChildItems.insert(parentId, {childGroupBoxes, childCards});
        }

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

void GroupBoxTree::reparentExistingGroupBox(const int groupBoxId, const int newParentId) {
    if (!groupBoxIdToParent.contains(groupBoxId)) {
        Q_ASSERT(false); // `groupBoxId` not found
        return;
    }

    if (!nodeIdToChildItems.contains(newParentId)) {
        Q_ASSERT(false); // `newParentId` not found
        return;
    }

    const int originalParentId = groupBoxIdToParent.value(groupBoxId);
    if (newParentId == originalParentId)
        return; // nothing need to be done

    if (newParentId == groupBoxId) {
        Q_ASSERT(false); // `newParentId` equals `groupBoxId`
        return;
    }

    if (isNodeDescendantOfNode(newParentId, groupBoxId)) {
        Q_ASSERT(false); // `newParentId` is a descendant of `groupBoxId`
        return;
    }

    nodeIdToChildItems[originalParentId].childGroupBoxes.remove(groupBoxId);
    nodeIdToChildItems[newParentId].childGroupBoxes << groupBoxId;
    groupBoxIdToParent[groupBoxId] = newParentId;
}

void GroupBoxTree::reparentExistingCard(const int cardId, const int newParentGroupBoxId) {
    if (newParentGroupBoxId == rootId) {
        Q_ASSERT(false); // root (the Board) cannot have child cards
        return;
    }

    if (!cardIdToParent.contains(cardId)) {
        Q_ASSERT(false); // `cardId` not found
        return;
    }

    if (!nodeIdToChildItems.contains(newParentGroupBoxId)) {
        Q_ASSERT(false); // `newParentId` not found
        return;
    }

    const int originalParentId = cardIdToParent.value(cardId);
    if (newParentGroupBoxId == originalParentId)
        return;

    nodeIdToChildItems[originalParentId].childCards.remove(cardId);
    nodeIdToChildItems[newParentGroupBoxId].childCards << cardId;
    cardIdToParent[cardId] = newParentGroupBoxId;
}

void GroupBoxTree::addOrReparentCard(const int cardId, const int newParentGroupBoxId) {
    if (cardIdToParent.contains(cardId))
        reparentExistingCard(cardId, newParentGroupBoxId);
    else
        addChildCards(newParentGroupBoxId, {cardId});
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
        for (const int groupBoxId: childItems.childGroupBoxes)
            groupBoxIdToParent[groupBoxId] = parentId;

        if (parentId != rootId) {
            nodeIdToChildItems[parentId].childCards += childItems.childCards;
            for (const int cardId: childItems.childCards)
                cardIdToParent[cardId] = parentId;
        }
        else {
            for (const int cardId: childItems.childCards)
                cardIdToParent.remove(cardId);
        }

        return;

    case RemoveOption::RemoveDescendants:
        for (const int cardId: childItems.childCards)
            cardIdToParent.remove(cardId);

        for (const int groupBoxId: childItems.childGroupBoxes) {
            const auto [descendantGroupBoxes, descendantCards] = breadthFirstTraversal(groupBoxId);
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

void GroupBoxTree::removeCardIfExists(const int cardIdToRemove) {
    if (cardIdToParent.contains(cardIdToRemove))
        removeCard(cardIdToRemove);
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

int GroupBoxTree::getParentGroupBoxOfCard(const int cardId) const {
    return cardIdToParent.value(cardId, -1);
}

bool GroupBoxTree::hasChild(const int constainerNodeId) const {
    return !nodeIdToChildItems.value(constainerNodeId).isEmpty();
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
    return breadthFirstTraversal(groupBoxId);
}

bool GroupBoxTree::formsSinglePath(const QSet<int> &groupBoxIds, int *deepestGroupBox) const {
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

    Q_ASSERT(!groupBoxesWithoutChildWithin.isEmpty());
    if (groupBoxesWithoutChildWithin.count() <= 1) {
        if (deepestGroupBox != nullptr)
            *deepestGroupBox = *groupBoxesWithoutChildWithin.constBegin();
        return true;
    }
    else {
        return false;
    }
}

QHash<int, QSet<int>> GroupBoxTree::getDescendantCardsOfEveryGroupBox(
        QVector<int> *groupBoxIdsFromDepthFirstTraversal) const {
    QHash<int, QSet<int>> groupBoxIdToCards;
    QVector<int> visitedGroupBoxes;
    depthFirstTraversal(
            [this, &groupBoxIdToCards, &visitedGroupBoxes](const QStack<int> &currentPath) {
                const int top = currentPath.top();
                if (top == rootId)
                    return;

                visitedGroupBoxes << top;

                const QSet<int> cards = nodeIdToChildItems[top].childCards;
                for (const int nodeId: currentPath) {
                    if (nodeId == rootId)
                        continue;
                    groupBoxIdToCards[nodeId] += cards;
                }
            }
    );

    if (groupBoxIdsFromDepthFirstTraversal != nullptr)
        *groupBoxIdsFromDepthFirstTraversal = visitedGroupBoxes;

    return groupBoxIdToCards;
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

void GroupBoxTree::addChildCards(const int parentGroupBoxId, const QSet<int> childCardIds) {
    if (parentGroupBoxId == rootId) {
        Q_ASSERT(false); // root cannot have child cards
        return;
    }

    if (!nodeIdToChildItems.contains(parentGroupBoxId)) {
        Q_ASSERT(false); // `parentId` is not found
        return;
    }

    if (!(childCardIds & keySet(cardIdToParent)).isEmpty()) {
        Q_ASSERT(false); // some of `childCardIds` already exist
        return;
    }

    //
    nodeIdToChildItems[parentGroupBoxId].childCards += childCardIds;

    for (const int id: childCardIds)
        cardIdToParent.insert(id, parentGroupBoxId);
}

bool GroupBoxTree::isNodeDescendantOfNode(
        const int containerNodeId1, const int containerNodeId2) const {
    if (!nodeIdToChildItems.contains(containerNodeId1)
            || !nodeIdToChildItems.contains(containerNodeId2)) {
        return false;
    }

    if (containerNodeId1 == containerNodeId2)
        return false;

    if (containerNodeId1 == rootId)
        return false;

    if (containerNodeId2 == rootId)
        return true;

    int currentNode = containerNodeId1;
    while (true) {
        int parent = groupBoxIdToParent.value(currentNode, -1);
        if (parent == -1) {
            Q_ASSERT(false);
            return false;
        }

        if (parent == containerNodeId2)
            return true;
        else if (parent == rootId)
            return false;
        currentNode = parent;
    }
}

std::pair<QSet<int>, QSet<int> > GroupBoxTree::breadthFirstTraversal(
        const int startContainerNodeId) const {
    Q_ASSERT(nodeIdToChildItems.contains(startContainerNodeId));

    QSet<int> groupBoxes;
    QSet<int> cards;
    breadthFirstSearch(
            startContainerNodeId,
            [this, &groupBoxes, &cards](const int currentContainerNodeId) {
                groupBoxes += nodeIdToChildItems[currentContainerNodeId].childGroupBoxes;
                cards += nodeIdToChildItems[currentContainerNodeId].childCards;
                return true;
            }
    );
    return {groupBoxes, cards};
}

void GroupBoxTree::breadthFirstSearch(
        const int startContainerNodeId, std::function<bool (const int)> action) const {
    Q_ASSERT(nodeIdToChildItems.contains(startContainerNodeId));
    Q_ASSERT(action);

    bool toContinue = action(startContainerNodeId);
    if (!toContinue)
        return;

    QVector<int> nodesToVisit {startContainerNodeId};
    QVector<int> foundInCurrentLevel;
    while (true) {
        foundInCurrentLevel.clear();
        for (const int id: qAsConst(nodesToVisit)) {
            const QSet<int> &childGroupBoxes = nodeIdToChildItems[id].childGroupBoxes;
            for (const int childId: childGroupBoxes) {
                foundInCurrentLevel << childId;
                toContinue = action(childId);
                if (!toContinue)
                    return;
            }
        }

        if (foundInCurrentLevel.isEmpty())
            break;

        nodesToVisit = foundInCurrentLevel;
    }
}

void GroupBoxTree::depthFirstTraversal(std::function<void (const QStack<int> &)> action) const {
    Q_ASSERT(action);

    QSet<int> visitedNodes;
    QStack<int> parentNodesStack;

    visitedNodes << rootId;
    parentNodesStack.push(rootId);
    action(parentNodesStack);


    while (!parentNodesStack.isEmpty()) {
        const int currentParent = parentNodesStack.top();

        // find a child group-box of `currentParent` that is not visited
        int unvisitedChild = -1;
        const QSet<int> childGroupBoxes = nodeIdToChildItems[currentParent].childGroupBoxes;
        for (const int id: childGroupBoxes) {
            if (!visitedNodes.contains(id)) {
                unvisitedChild = id;
                break;
            }
        }

        //
        if (unvisitedChild == -1) {
             // backtrack
            parentNodesStack.pop();
        }
        else {
            parentNodesStack.push(unvisitedChild);
            visitedNodes << unvisitedChild;
            action(parentNodesStack);
        }
    }
}

//======

bool GroupBoxTree::ConstContainerNode::isDescendantOfContainerNode(const int containerNodeId) {
    return treeConst->isNodeDescendantOfNode(nodeId, containerNodeId);
}

//======

void GroupBoxTree::ContainerNode::addChildGroupBoxes(const QSet<int> childGroupBoxIds) {
    tree->addChildGroupBoxes(nodeId, childGroupBoxIds);
}

void GroupBoxTree::ContainerNode::addChildCards(const QSet<int> childCardIds) {
    tree->addChildCards(nodeId, childCardIds);
}
