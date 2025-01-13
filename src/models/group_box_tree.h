#ifndef GROUPBOXTREE_H
#define GROUPBOXTREE_H

#include <QHash>
#include <QSet>

class GroupBoxTree
{
public:
    explicit GroupBoxTree();

    inline static const int rootId {-10}; // represents the root (i.e., the Board)

    //!
    //! \param parentId: can be the ID of an existing group-box or \c rootId
    //! \param childGroupBoxIds: must not already exist in the tree
    //!
    void addChildGroupBoxes(const int parentId, const QSet<int> childGroupBoxIds);

    //!
    //! \param parentId: can be the ID of an existing group-box or \c rootId
    //! \param childCardIds: must not already exist in the tree
    //!
    void addChildCards(const int parentId, const QSet<int> childCardIds);

    enum class RemoveOption {ReparentChildren, RemoveDescendants};

    //!
    //! \param groupBoxIdToRemove
    //! \param option:
    //!           \c ReparentChildren: reparent child items `groupBoxIdToRemove` to its parent
    //!           \c RemoveDescendants: remove all descendants of `groupBoxIdToRemove`
    //!
    void removeGroupBox(const int groupBoxIdToRemove, const RemoveOption option);

    void removeCard(const int cardIdToRemove);

    //

    //!
    //! \return Returns -1 if \e groupBoxId is not found. Returns \c rootId if the parent is the root.
    //!
    int getParentOfGroupBox(const int groupBoxId) const;

    //!
    //! \return Returns -1 if \e cardId is not found. Returns \c rootId if the parent is the root.
    //!
    int getParentOfCard(const int cardId) const;

    //!
    //! \return (group-boxes, cards), where group-boxes do not include \e groupBoxId itself.
    //!         Returns empty sets if \e groupBoxId is not found.
    //!
    std::pair<QSet<int>, QSet<int>> getAllDescendants(const int groupBoxId) const;

    //!
    //! Determine whether the set \e groupBoxIds of existing group-boxes forms a path (i.e., a
    //! linear structure).
    //! \param groupBoxIds: each group-box must already exist
    //! \return false if \e groupBoxIds is empty
    //!
    bool formsSinglePath(const QSet<int> &groupBoxIds) const;

private:
    struct ChildItems
    {
        QSet<int> childGroupBoxes;
        QSet<int> childCards;
    };
    QHash<int, ChildItems> nodeIdToChildItems; // keys are the existing group-boxes (plus `rootId`)

    QHash<int, int> groupBoxIdToParent; // keys are the existing group-boxes
    QHash<int, int> cardIdToParent; // keys are the existing cards

    //!
    //! \param groupBoxId: must exist
    //! \return (group-boxes, cards), where group-boxes do not include \e groupBoxId itself
    //!
    std::pair<QSet<int>, QSet<int>> doGetAllDescendants(const int groupBoxId) const;
};

#endif // GROUPBOXTREE_H
