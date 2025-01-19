#include <QHBoxLayout>
#include <QToolButton>
#include "app_data_readonly.h"
#include "right_sidebar_toolbar.h"
#include "services.h"
#include "widgets/app_style_sheet.h"

RightSidebarToolBar::RightSidebarToolBar(QWidget *parent)
        : SimpleToolBar(parent) {
    hLayout->addStretch();

    QToolButton *buttonCloseRightPanel;
    {
        buttonCloseRightPanel = new QToolButton;
        hLayout->addWidget(buttonCloseRightPanel);
        hLayout->setAlignment(buttonCloseRightPanel, Qt::AlignVCenter);

        buttonToIcon.insert(buttonCloseRightPanel, Icon::CloseRightPanel);
        buttonCloseRightPanel->setIconSize({24, 24});
        connect(buttonCloseRightPanel, &QToolButton::clicked, this, [this]() {
            emit closeRightSidebar();
        });
    }

    // styles
    setStyleClasses(buttonCloseRightPanel, {StyleClass::flatToolButton});

    //
    setUpButtonsWithIcons();
}

void RightSidebarToolBar::setUpButtonsWithIcons() {
    // set the icons with current theme
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = buttonToIcon.constBegin(); it != buttonToIcon.constEnd(); ++it)
        it.key()->setIcon(Icons::getIcon(it.value(), theme));

    // connect to "theme updated" signal
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {
        const auto theme = isDarkTheme ? Icons::Theme::Dark : Icons::Theme::Light;
        for (auto it = buttonToIcon.constBegin(); it != buttonToIcon.constEnd(); ++it)
            it.key()->setIcon(Icons::getIcon(it.value(), theme));
    });
}
