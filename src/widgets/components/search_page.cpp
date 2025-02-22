#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QTimer>
#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "search_page.h"
#include "services.h"
#include "utilities/action_debouncer.h"
#include "utilities/async_routine.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"
#include "widgets/app_style_sheet.h"
#include "widgets/components/custom_text_browser.h"
#include "widgets/components/search_bar.h"
#include "widgets/icons.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

SearchPage::SearchPage(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpConnections();

    //
    debouncedHandlerForResizeEvent = new ActionDebouncer(
            100,
            ActionDebouncer::Option::Delay,
            [this]() {
                resultBrowser->updateGeometry();
            },
            this
    );
}

void SearchPage::showEvent(QShowEvent *event) {
    QFrame::showEvent(event);
    searchBar->setFocus();
}

void SearchPage::resizeEvent(QResizeEvent *event) {
    QFrame::resizeEvent(event);
    debouncedHandlerForResizeEvent->tryAct();
}

void SearchPage::setUpWidgets() {
    auto *rootLayout = new QVBoxLayout;
    setLayout(rootLayout);
    rootLayout->setContentsMargins(0, 0, 4, 0); // <^>v
    rootLayout->setSpacing(2);
    {
        // search bar
        searchBar = new SearchBar;
        rootLayout->addWidget(searchBar);

        searchBar->setPlaceholderText("Search...");
        searchBar->setFontPointSize(11);

        // message
        labelMessage = new QLabel;
        rootLayout->addWidget(labelMessage);

        labelMessage->setWordWrap(true);

        // result
        resultBrowser = new CustomTextBrowser;
        rootLayout->addWidget(resultBrowser);

        resultBrowser->setOpenLinks(false);
        resultBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        //
        labelSearching = new QLabel("searching...");
        rootLayout->addWidget(labelSearching);

        labelSearching->setVisible(false);

        //
        rootLayout->addStretch(1);
    }

    //
    setStyleClasses(labelMessage, {StyleClass::mediumContrastTextColor});
    setStyleClasses(labelSearching, {StyleClass::mediumContrastTextColor});

    labelMessage->setStyleSheet(
            "QLabel {"
            "  margin-bottom: 4px;"
            "}"
    );
    resultBrowser->setStyleSheet(
            "QTextBrowser {"
            "  border: none;"
            "  font-size: 10.5pt;"
            "}"
    );
    labelSearching->setStyleSheet(
            "QLabel {"
            "  font-style: italic;"
            "}"
    );
}

void SearchPage::setUpConnections() {
    connect(searchBar, &SearchBar::edited, this, [this](const QString &text) {
        SearchData searchData = parseSearchText(text);
        labelMessage->setText(searchData.getMessage());
    });

    connect(searchBar, &SearchBar::submitted, this, [this](const QString &text) {
        SearchData searchData = parseSearchText(text);
        labelMessage->setText(searchData.getMessage());
        submitSearch(searchData);
    });

    //
    connect(resultBrowser, &CustomTextBrowser::anchorClicked, this, [this](const QUrl &link) {
        const auto [workspaceId, boardId, cardId] = parseUrlToNodeRect(link);
        qDebug() << workspaceId << boardId << cardId;
        if (workspaceId != -1)
            emit userToOpenBoard(workspaceId, boardId);
    });
}

void SearchPage::clearSearchResult() {
    resultBrowser->clear();
    relayoutOnResultBrowserContentsUpdated();
}

void SearchPage::relayoutOnResultBrowserContentsUpdated() {
    resultBrowser->updateGeometry();
    QTimer::singleShot(0, this, [this]() {
        resultBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        resultBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        // The scroll bar remains shown when the widget is expanded to just large enough to
        // accommodate the contents. Here we force the scroll bar to hide, and than show it if
        // needed.
    });
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

void SearchPage::submitSearch(const SearchData &searchData) {
    if (!isPerformingSearch)
        startSearch(searchData);
    else
        pendingSearchData = searchData;
}

void SearchPage::startSearch(const SearchData &searchData) {
    isPerformingSearch = true;
    labelSearching->setVisible(true);
    doSearch(
            searchData,
            // callbackOnFinish:
            [this]() {
                if (!pendingSearchData.has_value()) {
                    isPerformingSearch = false;
                    labelSearching->setVisible(false);
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
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Card cardData;
        QHash<int, Workspace> workspaces;
        QVector<int> workspacesList;
        int currentBoardId {-1}; // can be -1
        SearchCardIdResult resultFromCache;
        SearchCardIdResult completeResult;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine, cardId]() {
        // 1. query card
        Services::instance()->getAppDataReadonly()->queryCards(
                {cardId},
                // callback
                [routine, cardId](bool ok, const QHash<int, Card> &cards) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "search failed";
                        return;
                    }
                    if (!cards.contains(cardId)) {
                        context.setErrorFlag();
                        routine->errorMsg = QString("card %1 not found").arg(cardId);
                        return;
                    }
                    routine->cardData = cards.value(cardId);
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 2. get workspaces data
        Services::instance()->getAppDataReadonly()->getWorkspaces(
                [routine](bool ok, const QHash<int, Workspace> &workspaces) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "search failed";
                        return;
                    }
                    routine->workspaces = workspaces;
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 3. get workspaces list & current Board ID
        ContinuationContext context(routine);
        emit getWorkspaceIdsList(&routine->workspacesList);
        emit getCurrentBoardId(&routine->currentBoardId);

    }, this);

    routine->addStep([this, routine, cardId]() {
        // 4. search using only cached boards data
        ContinuationContext context(routine);

        const QHash<int, QString> foundBoardsIdToName
                = Services::instance()->getAppDataReadonly()->getBoardsShowingCardFromCache(cardId);

        routine->resultFromCache = SearchCardIdResult(
                cardId, routine->cardData.title, foundBoardsIdToName, routine->workspaces,
                routine->workspacesList, routine->currentBoardId);
        showSearchCardIdResult(routine->resultFromCache);
    }, this);

    routine->addStep([this, routine, cardId]() {
        // 5. search completely
        Services::instance()->getAppDataReadonly()->getBoardsShowingCard(
                cardId,
                // callback
                [routine, cardId](bool ok, const QHash<int, QString> &boardsIdToName) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "search failed";
                        return;
                    }

                    routine->completeResult = SearchCardIdResult(
                            cardId, routine->cardData.title, boardsIdToName, routine->workspaces,
                            routine->workspacesList, routine->currentBoardId);
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 6. show complete result
        if (!routine->resultFromCache.hasNoBoard()) {
            // show after delay
            showSearchCardIdResult(
                    routine->resultFromCache,
                    true // noLink
            );
            QTimer::singleShot(500, this, [this, routine]() {
                ContinuationContext context(routine);
                showSearchCardIdResult(routine->completeResult);
            });
        }
        else {
            // show immediately
            ContinuationContext context(routine);
            showSearchCardIdResult(routine->completeResult);
        }
    }, this);

    routine->addStep([this, routine, callbackOnFinish]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag) {
            resultBrowser->clear();
            resultBrowser->setPlainText(routine->errorMsg);
            relayoutOnResultBrowserContentsUpdated();
        }
        callbackOnFinish();
    }, this);

    routine->start();
}

void SearchPage::showSearchCardIdResult(const SearchCardIdResult &result, const bool noLink) {
    resultBrowser->clear();
    auto cursor = resultBrowser->textCursor();

    cursor.insertHtml(
            QString("Card %1 (<b>%2</b>)").arg(result.cardId).arg(result.cardTitle));
    {
        // set line height (won't work if this is before first `cursor.insertXxx()`.
        QTextBlockFormat format = cursor.blockFormat();
        format.setLineHeight(115, QTextBlockFormat::ProportionalHeight);
        cursor.setBlockFormat(format);
    }
    cursor.insertBlock();

    if (!result.hasNoBoard()) {
        cursor.insertText("is opened in the following boards:");
        cursor.insertBlock();
        cursor.insertBlock();
    }
    else {
        cursor.insertText("is not opened in any board.");
        cursor.insertBlock();
    }

    for (auto it = result.workspaceIdToFoundBoardIds.constBegin();
            it != result.workspaceIdToFoundBoardIds.constEnd(); ++it) {
        const int workspaceId = it.key();
        const QVector<int> &boardIds = it.value();

        const QString workspaceName = result.workspaceIdToName.value(workspaceId);
        cursor.insertHtml(QString("workspace <b>%1</b>").arg(workspaceName));
        cursor.setCharFormat({});
        if (workspaceId == result.currentWorkspaceId)
            cursor.insertText(" (current)");
        cursor.insertBlock();

        for (const int boardId: boardIds) {
            const QString boardName = result.boardIdToName.value(boardId);
            if (!noLink) {
                const QString link = createHyperLinkToNodeRect(
                        workspaceId, boardId, boardName, result.cardId);
                cursor.insertHtml(QString("- board %1").arg(link));
                cursor.setCharFormat({});
            }
            else {
                cursor.insertText(QString("- board %1").arg(boardName));
            }

            if (boardId == result.currentBoardId)
                cursor.insertText(" (current)");
            cursor.insertBlock();
        }
    }

    //
    resultBrowser->setTextCursor(cursor);
    relayoutOnResultBrowserContentsUpdated();
}

void SearchPage::searchTitleAndText(
        const QString &substring, std::function<void ()> callbackOnFinish) {
    callbackOnFinish();


}

QString SearchPage::createHyperLinkToNodeRect(
        const int workspaceId, const int boardId, const QString &boardName, const int cardId) {
    return QString("<a href=\"file:%1_%2_%3\">%4</a>")
            .arg(workspaceId).arg(boardId).arg(cardId).arg(boardName);
}

std::tuple<int, int, int> SearchPage::parseUrlToNodeRect(const QUrl &url) {
    static const QRegularExpression re(R"(^file:(\d+)_(\d+)_(\d+)$)");

    const auto m = re.match(url.toString());
    if (!m.hasMatch())
        return {-1, -1, -1};

    bool ok;
    const int workspaceId = m.captured(1).toInt(&ok);
    if (!ok)
        return {-1, -1, -1};
    const int boardId = m.captured(2).toInt(&ok);
    if (!ok)
        return {-1, -1, -1};
    const int cardId = m.captured(3).toInt(&ok);
    if (!ok)
        return {-1, -1, -1};

    return {workspaceId, boardId, cardId};
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

//====

SearchPage::SearchCardIdResult::SearchCardIdResult(
        const int cardId_, const QString &cardTitle_,
        const QHash<int, QString> &foundBoardsIdToName,
        const QHash<int, Workspace> &workspaces, const QVector<int> workspacesList,
        const int currentBoardId_) {
    // `cardId` & `cardTitle`
    cardId = cardId_;
    cardTitle = cardTitle_;

    // `workspaceIdToFoundBoardIds` & `currentWorkspaceId`
    currentWorkspaceId = -1;

    const QSet<int> allFoundBoardIds = keySet(foundBoardsIdToName);
    for (auto it = workspaces.constBegin(); it != workspaces.constEnd(); ++it) {
        if (it.value().boardIds.contains(currentBoardId_))
            currentWorkspaceId = it.key();

        //
        const QSet<int> foundBoards = it.value().boardIds & allFoundBoardIds;
        if (foundBoards.isEmpty())
            continue;

        QVector<int> boardsOrdering = it.value().boardsOrdering;
        if (foundBoards.contains(currentBoardId_)) {
            const int p = boardsOrdering.indexOf(currentBoardId_);
            if (p != -1 && p != 0)
                boardsOrdering.move(p, 0); // move current board to first
        }

        workspaceIdToFoundBoardIds.insert(
                it.key(),
                sortByOrdering(foundBoards, boardsOrdering, false)
        );
    }

    // `workspacesOrdering`
    workspacesOrdering = workspacesList;
    const int p = workspacesOrdering.indexOf(currentWorkspaceId);
    if (p != -1 && p != 0)
        workspacesOrdering.move(p, 0); // move current workspace to first

    // `workspaceIdToName`
    for (auto it = workspaces.constBegin(); it != workspaces.constEnd(); ++it)
        workspaceIdToName.insert(it.key(), it.value().name);

    // `boardIdToName`
    boardIdToName = foundBoardsIdToName;

    // `currentBoardId`
    currentBoardId = currentBoardId_;
}

bool SearchPage::SearchCardIdResult::hasNoBoard() const {
    return workspaceIdToFoundBoardIds.isEmpty();
}

