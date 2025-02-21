#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "search_page.h"
#include "services.h"
#include "utilities/async_routine.h"
#include "utilities/lists_vectors_util.h"
#include "widgets/app_style_sheet.h"
#include "widgets/components/search_bar.h"
#include "widgets/icons.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

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
    rootLayout->setSpacing(2);
    {
        // search bar
        searchBar = new SearchBar;
        rootLayout->addWidget(searchBar);

        // message
        labelMessage = new QLabel;
        rootLayout->addWidget(labelMessage);

        labelMessage->setWordWrap(true);

        // result
        resultBrowser = new QTextBrowser;
        rootLayout->addWidget(resultBrowser);

        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        policy.setVerticalStretch(1);
        resultBrowser->setSizePolicy(policy);

        resultBrowser->setOpenLinks(false);
    }

    //
    setStyleClasses(labelMessage, {StyleClass::mediumContrastTextColor});

    labelMessage->setStyleSheet(
            "QLabel {"
            "  margin-bottom: 4px;"
            "}"
    );
    resultBrowser->setStyleSheet(
            "QTextBrowser {"
            "  border: none;"
            "}"
    );
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

    //
    connect(resultBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl &link) {
        qDebug() << "link clicked:" << link;



    });
}

void SearchPage::clearSearchResult() {
    resultBrowser->clear();
    resultBrowser->setCurrentCharFormat({});
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
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Card cardData;
        int currentBoardId {-1}; // can be -1
        QSet<int> foundBoardIds;
        QHash<int, Workspace> workspaces;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, cardId]() {
        // get the card data
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

    routine->addStep([this, routine, cardId]() {
        emit getCurrentBoardId(&routine->currentBoardId);

        Services::instance()->getAppDataReadonly()->getBoardIdsShowingCard(
                cardId,
                // callback
                [routine](bool ok, const QSet<int> &boardIds) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "search failed";
                        return;
                    }
                    routine->foundBoardIds = boardIds;
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        if (routine->foundBoardIds.isEmpty()) {
            routine->nextStep();
            return;
        }

        // get workspaces data
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

    routine->addStep([this, routine, cardId]() {
        ContinuationContext context(routine);

        // get the list of boards where the card is shown, grouped by workspaces
        using WorkspaceAndBoards = std::pair<int, QVector<int>>;
        QVector<WorkspaceAndBoards> workspaceAndBoardsList;
        {
            int currentWorkspaceId = -1;
            QHash<int, QSet<int>> workspaceIdToFoundBoards;
            for (auto it = routine->workspaces.constBegin();
                    it != routine->workspaces.constEnd(); ++it) {
                const int workspaceId = it.key();
                const QSet<int> allBoardIds = it.value().boardIds;

                //
                if (allBoardIds.contains(routine->currentBoardId))
                    currentWorkspaceId = workspaceId;

                //
                const QSet<int> foundBoardIdInWorkspace = allBoardIds & routine->foundBoardIds;
                if (!foundBoardIdInWorkspace.isEmpty())
                    workspaceIdToFoundBoards.insert(workspaceId, foundBoardIdInWorkspace);
            }

            QVector<int> workspacesList;
            if (workspaceIdToFoundBoards.contains(currentWorkspaceId))
                workspacesList << currentWorkspaceId; // let current workspace be thw first
            for (auto it = workspaceIdToFoundBoards.constBegin();
                    it != workspaceIdToFoundBoards.constEnd(); ++it) {
                if (it.key() == currentWorkspaceId)
                    continue;
                workspacesList << it.key();
            }


            for (const int workspaceId: qAsConst(workspacesList)) {
                const QSet<int> foundBoardIds = workspaceIdToFoundBoards.value(workspaceId);

                //
                QVector<int> boardsOrdering = routine->workspaces.value(workspaceId).boardsOrdering;
                if (workspaceId == currentWorkspaceId) {
                    const int p = boardsOrdering.indexOf(routine->currentBoardId);
                    if (p != -1)
                        boardsOrdering.move(p, 0); // let current Board be the first
                }

                const QVector<int> boardsList = sortByOrdering(foundBoardIds, boardsOrdering, false);
                workspaceAndBoardsList.append(std::make_pair(workspaceId, boardsList));
            }
        }

        // update `resultBrowser`
        resultBrowser->clear();
        resultBrowser->setCurrentCharFormat({});
        auto cursor = resultBrowser->textCursor();

        cursor.insertHtml(
                QString("Card %1 (<b>%2</b>)").arg(cardId).arg(routine->cardData.title));
        cursor.insertBlock();

        if (!workspaceAndBoardsList.isEmpty()) {
            cursor.insertText("is opened in the following boards:");
            cursor.insertBlock();
            cursor.insertBlock();
        }
        else {
            cursor.insertText("is not opened in any board.");
            cursor.insertBlock();
        }

        for (const auto &[workspaceId, boardIds]: qAsConst(workspaceAndBoardsList)) {
            const QString workspaceName = routine->workspaces.value(workspaceId).name;
            cursor.insertHtml(QString("workspace <b>%1</b>").arg(workspaceName));
            cursor.insertBlock();

            for (const int boardId: boardIds) {
                const QString boardName = QString::number(boardId); // [temp]
                cursor.insertHtml(
                        QString("- board <a href=\"file:%1_%2_%3\">%4</a>")
                        .arg(workspaceId).arg(boardId).arg(cardId)
                        .arg(boardName)
                );
                cursor.insertBlock();
            }
        }

        resultBrowser->setTextCursor(cursor);
    }, this);

    routine->addStep([this, routine, callbackOnFinish]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag) {
            resultBrowser->clear();
            resultBrowser->setCurrentCharFormat({});
            resultBrowser->setPlainText(routine->errorMsg);
        }
        callbackOnFinish();
    }, this);

    routine->start();
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
