#ifndef GROUPBOXTREE_H
#define GROUPBOXTREE_H

#include <QHash>
#include <QSet>

//!
//! + Group-box ID and card ID must be >= 0.
//! + The root (the Board) cannot have child cards.
//!
class GroupBoxTree
{
public:
    struct Node {
        explicit Node(GroupBoxTree *tree, const int nodeId) : tree(tree), nodeId(nodeId) {}
        void addChildGroupBoxes(const QSet<int> childGroupBoxIds);
        void addChildCards(const QSet<int> childCardIds);
    private:
        GroupBoxTree *tree;
        int nodeId; // can be a group-box or root
    };

public:
    explicit GroupBoxTree();

    inline static const int rootId {-10}; // represents the root (i.e., the Board)

    //!
    //! The returned \c Node can be used to add child items to the node.
    //! \param nodeId: can be the ID of an existing group-box or \c rootId
    //!
    Node node(const int nodeId);

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
    //! \param nodeId: can be the ID of an existing group-box or \c rootId
    //! \return
    //!
    bool hasChild(const int nodeId) const;

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
    //! \param groupBoxId: must exist
    //! \return (group-boxes, cards), where group-boxes do not include \e groupBoxId itself
    //!
    std::pair<QSet<int>, QSet<int>> doGetAllDescendants(const int groupBoxId) const;
};

#endif // GROUPBOXTREE_H
