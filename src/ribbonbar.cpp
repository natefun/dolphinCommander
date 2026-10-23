/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ribbonbar.h"

#include <KActionMenu>
#include <KLocalizedString>
#include <KStandardAction>
#include <KToggleToolBarAction>

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
constexpr int IconSize = 24;
constexpr int CompactIconSize = 16;
constexpr int CompactStackSize = 3;
constexpr int PageMargin = 1;

// Menus without a tab. Go only repeats the Places panel and the recently closed tabs of the File tab.
const QStringList hiddenTabs{QStringLiteral("go"), QStringLiteral("help")};

// Actions that are not shown in the ribbon: the last tab is closed through the tab bar and the
// window through its title bar, the rest is of little use in a window that always has two sides.
const QStringList hiddenActions{QStringLiteral("file_close"),
                                QStringLiteral("file_quit"),
                                QStringLiteral("undo_close_tab"),
                                QStringLiteral("stop"),
                                QStringLiteral("focus_inactive_split_view"),
                                QStringLiteral("split_view"),
                                QStringLiteral("split_view_menu"),
                                QStringLiteral("split_stash"),
                                QStringLiteral("move_to_tabs_split_view"),
                                QStringLiteral("popout_split_view"),
                                QStringLiteral("options_show_toolbar"),
                                QStringLiteral("options_configure_toolbars")};

// Actions that are shown as small buttons, up to three on top of each other.
const QStringList compactActions{QStringLiteral("view_zoom_in"),
                                 QStringLiteral("view_zoom_reset"),
                                 QStringLiteral("view_zoom_out"),
                                 QStringLiteral("show_preview"),
                                 QStringLiteral("show_hidden_files"),
                                 QStringLiteral("act_as_admin"),
                                 QStringLiteral("restore_view_settings_default"),
                                 QStringLiteral("view_properties")};
}

RibbonBar::RibbonBar(QMenuBar *source, QWidget *parent)
    : QWidget(parent)
    , m_source(source)
{
    // The ribbon only takes the height it needs, the rest goes to the file views below it.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabBar = new QTabBar(this);
    m_tabBar->setDrawBase(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setFocusPolicy(Qt::NoFocus);
    m_tabRowLayout = new QHBoxLayout();
    m_tabRowLayout->setContentsMargins(4, 0, 0, 0);
    m_tabRowLayout->setSpacing(0);
    m_tabRowLayout->addWidget(m_tabBar);
    m_tabRowLayout->addStretch();
    layout->addLayout(m_tabRowLayout);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_scrollArea);

    const QList<QAction *> topLevelActions = source->actions();
    for (QAction *action : topLevelActions) {
        QMenu *menu = action->menu();
        if (!menu || action->isSeparator() || hiddenTabs.contains(menu->objectName()) || hiddenTabs.contains(action->objectName())) {
            continue;
        }
        m_menus.append(menu);
        m_tabBar->addTab(QString(action->text()).remove(QLatin1Char('&')));
    }

    connect(m_tabBar, &QTabBar::currentChanged, this, &RibbonBar::showPage);
    showPage(m_tabBar->currentIndex());
}

void RibbonBar::setLeadingActions(const QList<QAction *> &actions)
{
    int position = 0;
    for (QAction *action : actions) {
        auto *button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setIconSize(QSize(IconSize - 4, IconSize - 4));
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setFocusPolicy(Qt::NoFocus);
        button->setDefaultAction(action);
        if (action->menu()) {
            // Menus of actions that do something on their own, like going back, open on a long press.
            button->setPopupMode(qobject_cast<KActionMenu *>(action) ? QToolButton::InstantPopup : QToolButton::MenuButtonPopup);
        }
        m_tabRowLayout->insertWidget(position++, button);
    }
    m_tabRowLayout->insertSpacing(position, 8);
}

void RibbonBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Whether the page needs a scroll bar depends on the width.
    updateHeight();
}

void RibbonBar::updateHeight()
{
    QWidget *page = m_scrollArea->widget();
    if (!page) {
        return;
    }
    // All pages are as tall as a row of large buttons, so the ribbon does not change its height
    // from tab to tab. A scroll bar, if there is room for only part of the page, needs some more
    // room so that it does not cover the buttons.
    const bool needsScrollBar = page->sizeHint().width() > m_scrollArea->width();
    const int scrollBarHeight = needsScrollBar ? m_scrollArea->horizontalScrollBar()->sizeHint().height() : 0;
    m_scrollArea->setFixedHeight(m_pageHeight + scrollBarHeight + 2 * m_scrollArea->frameWidth());
}

int RibbonBar::largeButtonRowHeight() const
{
    // What a button with a large icon and its label under it needs.
    QToolButton probe;
    probe.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    probe.setIconSize(QSize(IconSize, IconSize));
    probe.setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    probe.setText(QStringLiteral("Mg"));
    probe.setAutoRaise(true);
    return probe.sizeHint().height() + 2 * PageMargin;
}

void RibbonBar::showPage(int index)
{
    m_pageHeight = largeButtonRowHeight();
    if (index < 0 || index >= m_menus.size() || !m_menus.at(index)) {
        m_scrollArea->takeWidget();
        return;
    }
    QMenu *menu = m_menus.at(index);

    // Some menus fill themselves right before they are shown.
    Q_EMIT menu->aboutToShow();

    auto *page = new QWidget();
    auto *pageLayout = new QHBoxLayout(page);
    pageLayout->setContentsMargins(6, PageMargin, 6, PageMargin);
    pageLayout->setSpacing(2);

    // Collect what is shown, without actions that are hidden and without separators that would
    // stand at the start, at the end or next to another separator.
    QList<QAction *> items;
    const QList<QAction *> actions = menu->actions();
    for (QAction *action : actions) {
        if (action->isSeparator()) {
            if (!items.isEmpty() && !items.last()->isSeparator()) {
                items.append(action);
            }
            continue;
        }
        // Refresh is next to back and forward, in the row of the tabs.
        if (!action->isVisible() || qobject_cast<KToggleToolBarAction *>(action) || hiddenActions.contains(action->objectName())
            || action->objectName() == KStandardAction::name(KStandardAction::Redisplay)) {
            continue;
        }
        items.append(action);
    }
    while (!items.isEmpty() && items.last()->isSeparator()) {
        items.removeLast();
    }

    // Shorter labels for the buttons than the menus have. Only the text of the buttons changes.
    const QHash<QString, QString> shortLabels{
        {QStringLiteral("view_mode"), i18nc("@action:intoolbar", "View Mode")},
        {QStringLiteral("additional_info"), i18nc("@action:intoolbar", "Details")},
        {QStringLiteral("show_preview"), i18nc("@action:intoolbar", "Previews")},
        {QStringLiteral("show_hidden_files"), i18nc("@action:intoolbar", "Hidden Files")},
        {QStringLiteral("act_as_admin"), i18nc("@action:intoolbar", "Admin")},
        {QStringLiteral("restore_view_settings_default"), i18nc("@action:intoolbar", "Reset View")},
        {QStringLiteral("view_properties"), i18nc("@action:intoolbar", "Display Style")},
        {QStringLiteral("panels"), i18nc("@action:intoolbar", "Panels")},
        {QStringLiteral("help_contents"), i18nc("@action:intoolbar", "Handbook")},
    };

    QVBoxLayout *stack = nullptr;
    auto addButton = [&](QAction *action, bool compact) {
        const auto shortLabel = shortLabels.constFind(action->objectName());
        if (shortLabel != shortLabels.constEnd()) {
            action->setIconText(shortLabel.value());
        }
        auto *button = new QToolButton(page);
        button->setAutoRaise(true);
        button->setIconSize(compact ? QSize(CompactIconSize, CompactIconSize) : QSize(IconSize, IconSize));
        button->setToolButtonStyle(compact ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonTextUnderIcon);
        button->setDefaultAction(action);
        if (action->menu()) {
            button->setPopupMode(QToolButton::InstantPopup);
        }
        if (!compact) {
            pageLayout->addWidget(button);
            return;
        }
        if (!stack || stack->count() >= CompactStackSize) {
            stack = new QVBoxLayout();
            stack->setContentsMargins(0, 0, 0, 0);
            stack->setSpacing(0);
            pageLayout->addLayout(stack);
        }
        // Three of them have to fit into the height of one large button.
        button->setFixedHeight((m_pageHeight - 2 * PageMargin) / CompactStackSize);
        stack->addWidget(button);
    };

    for (QAction *action : std::as_const(items)) {
        if (action->isSeparator()) {
            stack = nullptr;
            auto *line = new QFrame(page);
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            pageLayout->addWidget(line);
            continue;
        }
        const bool compact = compactActions.contains(action->objectName());
        if (!compact) {
            stack = nullptr;
        }
        addButton(action, compact);
    }
    pageLayout->addStretch();

    // The scroll area deletes the page it showed before.
    m_scrollArea->setWidget(page);
    updateHeight();
}
