#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "search_page.h"
#include "services.h"
#include "widgets/app_style_sheet.h"
#include "widgets/components/search_bar.h"
#include "widgets/icons.h"

SearchPage::SearchPage(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpConnections();
}

void SearchPage::setUpWidgets() {
//    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();

    auto *rootLayout = new QVBoxLayout;
    setLayout(rootLayout);
    rootLayout->setContentsMargins(0, 0, 4, 0); // <^>v
    {
        // search bar
        searchBar = new SearchBar;
        rootLayout->addWidget(searchBar);

        // message
        labelMessage = new QLabel;
        rootLayout->addWidget(labelMessage);

        labelMessage->setWordWrap(true);

        // result


        //
        rootLayout->addStretch(1);
    }

    //
    setStyleClasses(labelMessage, {StyleClass::mediumContrastTextColor});
}

void SearchPage::setUpConnections() {
    connect(searchBar, &SearchBar::edited, this, [this](const QString &text) {
        SearchData searchData = parseSearchText(text);
        labelMessage->setText(searchData.getMessage());
    });

    connect(searchBar, &SearchBar::editingFinished, this, [this](const QString &text) {
        SearchData searchData = parseSearchText(text);
        labelMessage->setText(searchData.getMessage());
        submitSearch(searchData);
    });
}


void SearchPage::clearSearchResult() {
    // todo ...


}

void SearchPage::submitSearch(const SearchData &searchData) {
    if (!isPerformingSearch)
        startSearch(searchData);
    else
        pendingSearchData = searchData;
}

void SearchPage::startSearch(const SearchData &searchData) {
    isPerformingSearch = true;
    doSearch(
            searchData,
            // callback
            [this]() {
                if (!pendingSearchData.has_value()) {
                    isPerformingSearch = false;
                }
                else {
                    const auto searchData = pendingSearchData.value();
                    pendingSearchData.reset();
                    startSearch(searchData);
                }
            }
    );
}

void SearchPage::doSearch(const SearchData &searchData, std::function<void ()> callbackOnFinish) {
    switch (searchData.getType()) {
    case SearchData::Type::None:
        clearSearchResult();
        callbackOnFinish();
        return;

    case SearchData::Type::CardId:
        searchCardId(
                searchData.getCardId(),
                [callbackOnFinish]() { callbackOnFinish(); }
        );
        return;

    case SearchData::Type::TitleAndText:
        searchTitleAndText(
                searchData.getSubstring(),
                [callbackOnFinish]() { callbackOnFinish(); }
        );
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void SearchPage::searchCardId(const int cardId, std::function<void ()> callbackOnFinish) {
    int workspaceId = -1;
    emit getCurrentWorkspaceId(&workspaceId);




}

void SearchPage::searchTitleAndText(
        const QString &substring, std::function<void ()> callbackOnFinish) {





}


SearchPage::SearchData SearchPage::parseSearchText(const QString &searchText_) {
    const QString searchText = searchText_.trimmed();

    if (searchText.isEmpty())
        return SearchData();

    static const QRegularExpression regexpForIdSearch(R"(^id:(.*)$)");
    const auto m = regexpForIdSearch.match(searchText);

    if (m.hasMatch()) {
        // search card ID's
        bool ok;
        const int cardId = m.captured(1).toInt(&ok);
        if (!ok)
            return SearchData();

        return SearchData::ofCardIdSearch(cardId);
    }
    else {
        // search titles or texts
        if (searchText.count() < 3)
            return SearchData();

        return SearchData::ofTitleAndTextSearch(searchText);
    }
}

//====

SearchPage::SearchData SearchPage::SearchData::ofCardIdSearch(const int cardId) {
    if (cardId < 0) {
        Q_ASSERT(false);
        return SearchData();
    }

    SearchData data;
    data.type = SearchData::Type::CardId;
    data.cardId = cardId;

    return data;
}

SearchPage::SearchData SearchPage::SearchData::ofTitleAndTextSearch(const QString &substring) {
    if (substring.isEmpty()) {
        Q_ASSERT(false);
        return SearchData();
    }

    SearchData data;
    data.type = SearchData::Type::TitleAndText;
    data.substring = substring;

    return data;
}

int SearchPage::SearchData::getCardId() const {
    Q_ASSERT(type == Type::CardId);
    return cardId;
}

QString SearchPage::SearchData::getSubstring() const {
    Q_ASSERT(type == Type::TitleAndText);
    return substring;
}

QString SearchPage::SearchData::getMessage() const {
    switch (type) {
    case Type::None:
        return "";

    case Type::CardId:
        Q_ASSERT(cardId >= 0);
        return QString("Match card ID %1").arg(cardId);

    case Type::TitleAndText:
        return QString("Search titles and texts with keyword \"%1\"").arg(substring);
    }
    Q_ASSERT(false); // case not implemented
    return "";
}
