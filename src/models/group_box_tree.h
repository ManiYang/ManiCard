#ifndef GROUPBOXTREE_H
#define GROUPBOXTREE_H

#include <functional>
#include <QHash>
#include <QSet>
#include <QStack>

//!
//! + Group-box ID and card ID must be >= 0.
//! + The root (the Board) cannot have child cards.
//! + A "container node" is a group-box or the root (Board).
//!
class GroupBoxTree
{
public:
    struct ConstContainerNode
    {
        explicit ConstContainerNode(GroupBoxTree *tree, const int containerNodeId)
            : treeConst(tree), nodeId(containerNodeId) {}
        bool isDescendantOfContainerNode(const int containerNodeId);

    protected:
        const GroupBoxTree *treeConst;
        int nodeId; // can be a group-box or root
    };

    struct ContainerNode : public ConstContainerNode
    {
        explicit ContainerNode(GroupBoxTree *tree, const int containerNodeId)
            : ConstContainerNode(tree, containerNodeId), tree(tree) {}
        void addChildGroupBoxes(const QSet<int> childGroupBoxIds);
        void addChildCards(const QSet<int> childCardIds);

    private:
        GroupBoxTree *tree;
    };

public:
    explicit GroupBoxTree();

    inline static const int rootId {-10}; // represents the root (i.e., the Board)

    //!
    //! The returned \c ContainerNode can be used to add child items to the node or do queries
    //! about the node.
    //! \param nodeId: can be the ID of an existing group-box or \c rootId
    //!
    ContainerNode containerNode(const int nodeId);
    ConstContainerNode containerNode(const int nodeId) const;

    using ChildGroupBoxesAndCards = std::pair<QSet<int>, QSet<int>>;

    //!
    //! Sets up the tree from \e groupBoxIdToChildItems.
    //!   - Any group-boxes that is not given a parent will be a child of root (the Board). So,
    //!     it is OK not to provide the child group-boxes of the root (Board).
    //!   - Note that the root (Board) cannot have child cards.
    //! \return true if successful
    //!
    bool set(const QHash<int, ChildGroupBoxesAndCards> &groupBoxIdToChildItems, QString *errorMsg);

    //!
    //! \param groupBoxId
    //! \param newParentId: can be the ID of an existing group-box or \c rootId, but must not
    //!                     be `groupBoxId` or its descendant
    //!
    void reparentExistingGroupBox(const int groupBoxId, const int newParentId);

    void reparentExistingCard(const int cardId, const int newParentGroupBoxId);
    void addOrReparentCard(const int cardId, const int newParentGroupBoxId);

    enum class RemoveOption {ReparentChildren, RemoveDescendants};

    //!
    //! \param groupBoxIdToRemove
    //! \param option:
    //!           \c ReparentChildren: Reparent child items of `groupBoxIdToRemove` to its
    //!                   parent. If `groupBoxIdToRemove`'s parent is root, its child cards
    //!                   are removed (since root cannot have child cards).
    //!           \c RemoveDescendants: Remove all descendants of `groupBoxIdToRemove`
    //!
    void removeGroupBox(const int groupBoxIdToRemove, const RemoveOption option);

    void removeCard(const int cardIdToRemove);
    void removeCardIfExists(const int cardIdToRemove);

    //!
    //! Remove all group-boxes and cards. The root (Board) remains.
    //!
    void clear();

    //

    int getGroupBoxesCount() const;
    int getCardsCount() const;

    //!
    //! \return Returns -1 if \e groupBoxId is not found. Returns \c rootId if the parent is the root.
    //!
    int getParentOfGroupBox(const int groupBoxId) const;

    //!
    //! \return Returns -1 if \e cardId is not found.
    //!
    int getParentGroupBoxOfCard(const int cardId) const;

    //!
    //! \param constainerNodeId: can be the ID of an existing group-box or \c rootId
    //! \return
    //!
    bool hasChild(const int constainerNodeId) const;

    //!
    //! \param parentId: can be the ID of an existing group-box or \c rootId
    //!
    QSet<int> getChildGroupBoxes(const int parentId) const;

    //!
    //! \param parentId: can be the ID of an existing group-box or \c rootId
    //!
    QSet<int> getChildCards(const int parentId) const;

    //!
    //! \return (group-boxes, cards), where group-boxes do not include \e groupBoxId itself.
    //!         Returns empty sets if \e groupBoxId is not found.
    //!
    std::pair<QSet<int>, QSet<int>> getAllDescendants(const int groupBoxId) const;

    //!
    //! Determine whether the set \e groupBoxIds of existing group-boxes forms a path (i.e., a
    //! linear structure).
    //! \param groupBoxIds: each group-box must already exist
    //! \param deepestGroupBox: if \e groupBoxIds does form a path, \e deepestGroupBox will be
    //!                         set to the ID of the deepest group box
    //! \return false if \e groupBoxIds is empty
    //!
    bool formsSinglePath(const QSet<int> &groupBoxIds, int *deepestGroupBox = nullptr) const;

    //!
    //! \return a QHash that maps group box ID to the set of its descendant cards
    //!
    QHash<int, QSet<int>> getDescendantCardsOfEveryGroupBox(
            QVector<int> *groupBoxIdsFromDepthFirstTraversal = nullptr) const;

private:
    struct ChildItems
    {
        QSet<int> childGroupBoxes;
        QSet<int> childCards;

        bool isEmpty() const { return childGroupBoxes.isEmpty() && childCards.isEmpty(); }
    };
    QHash<int, ChildItems> nodeIdToChildItems; // keys are the existing group-boxes, plus `rootId`

    QHash<int, int> groupBoxIdToParent; // keys are the existing group-boxes
    QHash<int, int> cardIdToParent; // keys are the existing cards

    //!
    //! \param parentId: can be the ID of an existing group-box or \c rootId
    //! \param childGroupBoxIds: must not already exist in the tree
    //!
    void addChildGroupBoxes(const int parentId, const QSet<int> childGroupBoxIds);

    //!
    //! \param parentGroupBoxId: can be the ID of an existing group-box
    //! \param childCardIds: must not already exist in the tree
    //!
    void addChildCards(const int parentGroupBoxId, const QSet<int> childCardIds);

    //!
    //! \param containerNodeId1: group-box ID or \c rootId
    //! \param containerNodeId2: group-box ID or \c rootId
    //! \return whether \e containerNodeId1 is a descendant of \e containerNodeId2 (false if they
    //!         are the same node). Returns false if either node is not found.
    //!
    bool isNodeDescendantOfNode(const int containerNodeId1, const int containerNodeId2) const;

    //!
    //! \param startContainerNodeId: starting group-box ID (must exist) or \c rootId
    //! \return (group-boxes, cards), where group-boxes do not include \e containerNodeId itself
    //!
    std::pair<QSet<int>, QSet<int>> breadthFirstTraversal(const int startContainerNodeId) const;

    //!
    //! \param startContainerNodeId: starting group-box ID (must exist) or \c rootId
    //! \param action:
    //!          + This function will be called when a containter node is visited (including
    //!            \e startContainerNodeId itself)
    //!          + If this function returns false, the search stops.
    //!          + This function must not modify the tree.
    //!
    void breadthFirstSearch(
            const int startContainerNodeId,
            std::function<bool (const int currentContainerNodeId)> action) const;

    //!
    //! Do depth-first traversal from the root.
    //! \param action:
    //!          + This function will be called when a container node (including the root) is
    //!            visited.
    //!          + The parameter \e currentPath contains the current container node and all its
    //!            predecessors. Its \c top() or \c last() is the current container node, and its
    //!            \c first() is the root (the Board).
    //!          + This function must not modify the tree.
    //!
    void depthFirstTraversal(
            std::function<void (const QStack<int> &currentPath)> action) const;
};

#endif // GROUPBOXTREE_H
