#ifndef SEARCHPAGE_H
#define SEARCHPAGE_H

#include <functional>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>

class SearchBar;

class SearchPage : public QFrame
{
    Q_OBJECT
public:
    explicit SearchPage(QWidget *parent = nullptr);

signals:
    void getCurrentWorkspaceId(int *workspaceId);

private:
    SearchBar *searchBar {nullptr};
    QLabel *labelMessage {nullptr};

    void setUpWidgets();
    void setUpConnections();

    void clearSearchResult();





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

    void submitSearch(const SearchData &searchData);
    void startSearch(const SearchData &searchData);
    void doSearch(const SearchData &searchData, std::function<void ()> callbackOnFinish);
    void searchCardId(const int cardId, std::function<void ()> callbackOnFinish);
    void searchTitleAndText(const QString &substring, std::function<void ()> callbackOnFinish);

    static SearchData parseSearchText(const QString &searchText);
};

#endif // SEARCHPAGE_H
