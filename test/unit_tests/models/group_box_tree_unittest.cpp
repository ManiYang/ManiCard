#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "models/group_box_tree.h"

TEST(GroupBoxTreeTests, addChildItemsAndGet) {
    // create tree
    GroupBoxTree tree;
    ASSERT_TRUE(tree.getGroupBoxesCount() == 0);
    ASSERT_TRUE(tree.getCardsCount() == 0);

    tree.containerNode(GroupBoxTree::rootId).addChildGroupBoxes({1, 4});
    ASSERT_TRUE(tree.getGroupBoxesCount() == 2);

    tree.containerNode(1).addChildCards({11});

    tree.containerNode(1).addChildGroupBoxes({2, 3});
    ASSERT_TRUE(tree.getGroupBoxesCount() == 4);

    tree.containerNode(2).addChildCards({12});
    tree.containerNode(3).addChildCards({13});
    tree.containerNode(4).addChildCards({14});
    ASSERT_TRUE(tree.getCardsCount() == 4);

    //
    ASSERT_TRUE(tree.getParentOfGroupBox(1) == GroupBoxTree::rootId);
    ASSERT_TRUE(tree.getParentOfGroupBox(2) == 1);
    ASSERT_TRUE(tree.getParentOfGroupBox(3) == 1);
    ASSERT_TRUE(tree.getParentOfGroupBox(4) == GroupBoxTree::rootId);
    ASSERT_TRUE(tree.getParentOfGroupBox(10000) == -1);

    ASSERT_TRUE(tree.getParentGroupBoxOfCard(11) == 1);
    ASSERT_TRUE(tree.getParentGroupBoxOfCard(12) == 2);
    ASSERT_TRUE(tree.getParentGroupBoxOfCard(13) == 3);
    ASSERT_TRUE(tree.getParentGroupBoxOfCard(14) == 4);
    ASSERT_TRUE(tree.getParentGroupBoxOfCard(99) == -1);

    //
    ASSERT_TRUE(tree.getChildGroupBoxes(GroupBoxTree::rootId) == (QSet<int> {1, 4}));
    ASSERT_TRUE(tree.getChildGroupBoxes(1) == (QSet<int> {2, 3}));
    ASSERT_TRUE(tree.getChildGroupBoxes(2) == (QSet<int> {}));
    ASSERT_TRUE(tree.getChildGroupBoxes(3) == (QSet<int> {}));
    ASSERT_TRUE(tree.getChildGroupBoxes(4) == (QSet<int> {}));

    ASSERT_TRUE(tree.getChildCards(1) == (QSet<int> {11}));
    ASSERT_TRUE(tree.getChildCards(2) == (QSet<int> {12}));
    ASSERT_TRUE(tree.getChildCards(3) == (QSet<int> {13}));
    ASSERT_TRUE(tree.getChildCards(4) == (QSet<int> {14}));

    //
    auto [groupBoxes, cards] = tree.getAllDescendants(GroupBoxTree::rootId);
    EXPECT_TRUE(groupBoxes == (QSet<int> {1, 2, 3, 4}));
    EXPECT_TRUE(cards == (QSet<int> {11, 12, 13, 14}));

    std::tie(groupBoxes, cards) = tree.getAllDescendants(1);
    EXPECT_TRUE(groupBoxes == (QSet<int> {2, 3}));
    EXPECT_TRUE(cards == (QSet<int> {11, 12, 13}));

    std::tie(groupBoxes, cards) = tree.getAllDescendants(2);
    EXPECT_TRUE(groupBoxes == (QSet<int> {}));
    EXPECT_TRUE(cards == (QSet<int> {12}));

    //
    EXPECT_TRUE(tree.formsSinglePath(QSet<int> {1, 2}));
    EXPECT_TRUE(tree.formsSinglePath(QSet<int> {1}));
    EXPECT_FALSE(tree.formsSinglePath(QSet<int> {2, 3}));
    EXPECT_FALSE(tree.formsSinglePath(QSet<int> {1, 4}));
    EXPECT_FALSE(tree.formsSinglePath(QSet<int> {1, 2, 3}));
    EXPECT_FALSE(tree.formsSinglePath(QSet<int> {1, 2, 4}));
    EXPECT_FALSE(tree.formsSinglePath(QSet<int> {}));

    //
    EXPECT_TRUE(tree.containerNode(2).isDescendantOfContainerNode(GroupBoxTree::rootId));
    EXPECT_TRUE(tree.containerNode(2).isDescendantOfContainerNode(1));
    EXPECT_FALSE(tree.containerNode(2).isDescendantOfContainerNode(2));
    EXPECT_FALSE(tree.containerNode(1).isDescendantOfContainerNode(2));
    EXPECT_FALSE(tree.containerNode(GroupBoxTree::rootId).isDescendantOfContainerNode(2));

    //
    QVector<int> groupBoxesFromDFS;
    const QHash<int, QSet<int>> groupBoxIdToDescendantCards
             = tree.getDescendantCardsOfEveryGroupBox(&groupBoxesFromDFS);

    EXPECT_TRUE(
            groupBoxesFromDFS == (QVector<int> {1, 2, 3, 4})
            || groupBoxesFromDFS == (QVector<int> {1, 3, 2, 4})
            || groupBoxesFromDFS == (QVector<int> {1, 4, 2, 3})
            || groupBoxesFromDFS == (QVector<int> {4, 1, 2, 3})
            || groupBoxesFromDFS == (QVector<int> {4, 1, 3, 2})
    );

    EXPECT_TRUE(groupBoxIdToDescendantCards[1] == (QSet<int> {11, 12, 13}));
    EXPECT_TRUE(groupBoxIdToDescendantCards[2] == (QSet<int> {12}));
    EXPECT_TRUE(groupBoxIdToDescendantCards[3] == (QSet<int> {13}));
    EXPECT_TRUE(groupBoxIdToDescendantCards[4] == (QSet<int> {14}));
}

TEST(GroupBoxTreeTests, RemoveItems) {
    // create tree
    GroupBoxTree tree;
    tree.containerNode(GroupBoxTree::rootId).addChildGroupBoxes({1, 4});
    {
        tree.containerNode(1).addChildCards({11});
        tree.containerNode(1).addChildGroupBoxes({2, 3});
        {
            tree.containerNode(2).addChildCards({12});
            tree.containerNode(3).addChildCards({13});
            tree.containerNode(3).addChildGroupBoxes({5});
            {
                tree.containerNode(5).addChildCards({15});
            }
        }
        tree.containerNode(4).addChildCards({14});
    }

    //
    ASSERT_TRUE(tree.getGroupBoxesCount() == 5);
    ASSERT_TRUE(tree.getCardsCount() == 5);

    ASSERT_TRUE(tree.formsSinglePath(QSet<int> {1, 3, 5}));

    auto [groupBoxes, cards] = tree.getAllDescendants(1);
    EXPECT_TRUE(groupBoxes == (QSet<int> {2, 3, 5}));
    EXPECT_TRUE(cards == (QSet<int> {11, 12, 13, 15}));

    //
    tree.removeCard(12);
    EXPECT_TRUE(tree.getGroupBoxesCount() == 5);
    EXPECT_TRUE(tree.getCardsCount() == 4);
    EXPECT_TRUE(tree.getChildCards(2).isEmpty());

    tree.removeGroupBox(4, GroupBoxTree::RemoveOption::RemoveDescendants);
    EXPECT_TRUE(tree.getGroupBoxesCount() == 4);
    EXPECT_TRUE(tree.getCardsCount() == 3);
    EXPECT_TRUE(tree.getChildGroupBoxes(GroupBoxTree::rootId) == (QSet<int> {1}));

    tree.removeGroupBox(1, GroupBoxTree::RemoveOption::ReparentChildren);
    EXPECT_TRUE(tree.getGroupBoxesCount() == 3);
    EXPECT_TRUE(tree.getCardsCount() == 2);
    EXPECT_TRUE(tree.getChildGroupBoxes(GroupBoxTree::rootId) == (QSet<int> {2, 3}));
    EXPECT_EQ(tree.getChildCards(GroupBoxTree::rootId).count(), 0);
}

TEST(GroupBoxTreeTests, SetSuccess1) {
    using ChildGroupBoxesAndCards = std::pair<QSet<int>, QSet<int>>;
    const QHash<int, ChildGroupBoxesAndCards> nodeToChildItems {
        {2, ChildGroupBoxesAndCards {{}, {12}}},
        {4, ChildGroupBoxesAndCards {{}, {14}}},
        {1, ChildGroupBoxesAndCards {{2, 3}, {11}}},
        {GroupBoxTree::rootId, ChildGroupBoxesAndCards {{1, 4}, {}}},
        {3, ChildGroupBoxesAndCards {{}, {13}}},
    };

    GroupBoxTree tree;
    QString errorMsg;
    bool ok = tree.set(nodeToChildItems, &errorMsg);
    ASSERT_TRUE(ok);

    ASSERT_TRUE(tree.getGroupBoxesCount() == 4);
    ASSERT_TRUE(tree.getCardsCount() == 4);

    auto [groupBoxes, cards] = tree.getAllDescendants(GroupBoxTree::rootId);
    EXPECT_TRUE(groupBoxes == (QSet<int> {1, 2, 3, 4}));
    EXPECT_TRUE(cards == (QSet<int> {11, 12, 13, 14}));

    EXPECT_TRUE(tree.formsSinglePath(QSet<int> {1, 2}));
}

TEST(GroupBoxTreeTests, SetSuccess2) {
    using ChildGroupBoxesAndCards = std::pair<QSet<int>, QSet<int>>;
    const QHash<int, ChildGroupBoxesAndCards> nodeToChildItems {
        {GroupBoxTree::rootId, ChildGroupBoxesAndCards {{1, 4}, {}}},
    };

    GroupBoxTree tree;
    QString errorMsg;
    bool ok = tree.set(nodeToChildItems, &errorMsg);
    ASSERT_TRUE(ok);

    ASSERT_TRUE(tree.getGroupBoxesCount() == 2);
    ASSERT_TRUE(tree.getCardsCount() == 0);

    ASSERT_TRUE(tree.getChildGroupBoxes(1).isEmpty());
}

TEST(GroupBoxTreeTests, SetSuccess3) {
    using ChildGroupBoxesAndCards = std::pair<QSet<int>, QSet<int>>;
    const QHash<int, ChildGroupBoxesAndCards> nodeToChildItems {
        {GroupBoxTree::rootId, ChildGroupBoxesAndCards {{1}, {}}},
        {2, ChildGroupBoxesAndCards {{3}, {}}},
    };

    GroupBoxTree tree;
    QString errorMsg;
    bool ok = tree.set(nodeToChildItems, &errorMsg);
    ASSERT_TRUE(ok);

    ASSERT_TRUE(tree.getChildGroupBoxes(GroupBoxTree::rootId) == (QSet<int> {1, 2}));
}

TEST(GroupBoxTreeTests, SetFail1) {
    using ChildGroupBoxesAndCards = std::pair<QSet<int>, QSet<int>>;
    const QHash<int, ChildGroupBoxesAndCards> nodeToChildItems {
        {GroupBoxTree::rootId, ChildGroupBoxesAndCards {{1, 4}, {}}},
        {1, ChildGroupBoxesAndCards {{3}, {}}},
        {4, ChildGroupBoxesAndCards {{3}, {}}}, // forms a loop
    };

    GroupBoxTree tree;
    QString errorMsg;
    bool ok = tree.set(nodeToChildItems, &errorMsg);
    ASSERT_FALSE(ok);
}

TEST(GroupBoxTreeTests, SetFail2) {
    using ChildGroupBoxesAndCards = std::pair<QSet<int>, QSet<int>>;
    const QHash<int, ChildGroupBoxesAndCards> nodeToChildItems {
        {GroupBoxTree::rootId, ChildGroupBoxesAndCards {{1, 4}, {10}}}, // root has child card
    };

    GroupBoxTree tree;
    QString errorMsg;
    bool ok = tree.set(nodeToChildItems, &errorMsg);
    ASSERT_FALSE(ok);
}

TEST(GroupBoxTreeTests, Reparent) {
    // create tree
    GroupBoxTree tree;
    tree.containerNode(GroupBoxTree::rootId).addChildGroupBoxes({1, 4});
    {
        tree.containerNode(1).addChildCards({11});
        tree.containerNode(1).addChildGroupBoxes({2, 3});
        {
            tree.containerNode(2).addChildCards({12});
            tree.containerNode(3).addChildCards({13});
            tree.containerNode(3).addChildGroupBoxes({5});
            {
                tree.containerNode(5).addChildCards({15});
            }
        }
        tree.containerNode(4).addChildCards({14});
    }

    ASSERT_TRUE(tree.getGroupBoxesCount() == 5);
    ASSERT_TRUE(tree.getCardsCount() == 5);

    //
    tree.reparentExistingCard(15, 1);
    EXPECT_TRUE(tree.getChildCards(1) == (QSet<int> {11, 15}));
    EXPECT_TRUE(tree.getChildCards(5).isEmpty());

    //
    tree.reparentExistingGroupBox(3, GroupBoxTree::rootId);
    EXPECT_TRUE(tree.getChildGroupBoxes(GroupBoxTree::rootId) == (QSet<int> {1, 3, 4}));
    EXPECT_TRUE(tree.getChildGroupBoxes(1) == (QSet<int> {2}));

    //
    tree.reparentExistingGroupBox(4, 5);
    EXPECT_TRUE(tree.getChildGroupBoxes(GroupBoxTree::rootId) == (QSet<int> {1, 3}));
    EXPECT_TRUE(tree.formsSinglePath({3, 4, 5}));
}
