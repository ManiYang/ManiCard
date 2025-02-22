#ifndef SEARCHPAGE_H
#define SEARCHPAGE_H

#include <functional>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include "models/workspace.h"

class ActionDebouncer;
class CustomTextBrowser;
class SearchBar;

class SearchPage : public QFrame
{
    Q_OBJECT
public:
    explicit SearchPage(QWidget *parent = nullptr);

signals:
    void getCurrentBoardId(int *boardId);
    void getWorkspaceIdsList(QVector<int> *workspaceIds);

    void userToOpenBoard(const int workspaceId, const int boardId);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    SearchBar *searchBar {nullptr};
    QLabel *labelMessage {nullptr};
    CustomTextBrowser *resultBrowser {nullptr};
    QLabel *labelSearching {nullptr};

    ActionDebouncer *debouncedHandlerForResizeEvent;

    void setUpWidgets();
    void setUpConnections();

    void clearSearchResult();
    void relayoutOnResultBrowserContentsUpdated();

    //
    struct SearchData
    {
        enum class Type {
            None,
            CardId,      //!> search for card with ID = `cardId`
            TitleAndText //!> search for cards whose title or text contain `substring`
        };

        static SearchData ofCardIdSearch(const int cardId);
        static SearchData ofTitleAndTextSearch(const QString &substring);

        Type getType() const { return type; }
        int getCardId() const;
        QString getSubstring() const;

        QString getMessage() const;
    private:
        Type type {Type::None};
        int cardId {-1}; // for Type::CardId
        QString substring; // for Type::TitleAndText
    };

    bool isPerformingSearch {false};
    std::optional<SearchData> pendingSearchData;

    static SearchData parseSearchText(const QString &searchText);

    void submitSearch(const SearchData &searchData);
    void startSearch(const SearchData &searchData);
    void doSearch(const SearchData &searchData, std::function<void ()> callbackOnFinish);

    //
    struct SearchCardIdResult
    {
        explicit SearchCardIdResult() {}
        explicit SearchCardIdResult(
                const int cardId, const QString &cardTitle,
                const QHash<int, QString> &foundBoardsIdToName,
                const QHash<int, Workspace> &workspaces,
                const QVector<int> workspacesList, const int currentBoardId);
                // `currentBoardId` can be  -1

        bool hasNoBoard() const;

        //
        int cardId {-1};
        QString cardTitle;
        QHash<int, QVector<int>> workspaceIdToFoundBoardIds;
        QVector<int> workspacesOrdering;
        QHash<int, QString> workspaceIdToName;
        QHash<int, QString> boardIdToName;
        int currentWorkspaceId;
        int currentBoardId;
    };

    void searchCardId(const int cardId, std::function<void ()> callbackOnFinish);
    void showSearchCardIdResult(const SearchCardIdResult &result, const bool noLink = false);

    //
    void searchTitleAndText(const QString &substring, std::function<void ()> callbackOnFinish);

    //
    static QString createHyperLinkToNodeRect(
            const int workspaceId, const int boardId, const QString &boardName, const int cardId);
    static std::tuple<int, int, int> parseUrlToNodeRect(const QUrl &url);
            // returns (-1, -1, -1) if failed
};

#endif // SEARCHPAGE_H
