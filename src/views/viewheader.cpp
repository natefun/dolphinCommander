/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "viewheader.h"

#include "commandersettings.h"

#include "dolphinurlnavigator.h"
#include "views/dolphinview.h"

#include <KFileItem>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
std::function<void(QWidget *)> s_menuHandler;

QToolButton *createButton(QWidget *parent, const QString &iconName, const QString &toolTip)
{
    auto *button = new QToolButton(parent);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setIcon(QIcon::fromTheme(iconName));
    button->setToolTip(toolTip);
    return button;
}
}

ViewHeader::ViewHeader(DolphinView *view, DolphinUrlNavigator *navigator, QWidget *parent)
    : QWidget(parent)
    , m_view(view)
    , m_navigator(navigator)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(0);

    m_title = new QLabel(this);
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.6);
    m_title->setFont(titleFont);
    m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    layout->addWidget(m_title);

    m_counts = new QLabel(this);
    m_counts->setEnabled(false);
    layout->addWidget(m_counts);

    auto *row = new QHBoxLayout;
    row->setSpacing(0);
    layout->addLayout(row);

    auto *menuButton = createButton(this, QStringLiteral("application-menu"), i18nc("@info:tooltip", "Menu"));
    row->addWidget(menuButton);
    row->addSpacing(4);
    connect(menuButton, &QToolButton::clicked, this, [menuButton]() {
        if (s_menuHandler) {
            s_menuHandler(menuButton);
        }
    });

    m_back = createButton(this, QStringLiteral("go-previous"), i18nc("@info:tooltip", "Back"));
    m_forward = createButton(this, QStringLiteral("go-next"), i18nc("@info:tooltip", "Forward"));
    m_up = createButton(this, QStringLiteral("go-up"), i18nc("@info:tooltip", "Up"));
    row->addWidget(m_back);
    row->addWidget(m_forward);
    row->addWidget(m_up);
    row->addSpacing(8);

    connect(m_back, &QToolButton::clicked, this, [this]() {
        m_navigator->goBack();
        if (m_navigator->locationState().isEmpty()) {
            // An empty location state indicates a redirection URL, which must be skipped too.
            m_navigator->goBack();
        }
    });
    connect(m_forward, &QToolButton::clicked, m_navigator, &KUrlNavigator::goForward);
    connect(m_up, &QToolButton::clicked, m_navigator, &KUrlNavigator::goUp);
    connect(m_navigator, &KUrlNavigator::historyChanged, this, &ViewHeader::updateNavigation);
    connect(CommanderSettings::notifier(), &CommanderSettings::Notifier::changed, this, [this](const QString &key) {
        if (key == QLatin1String("ShowFilesViewToolbar")) {
            updateModes();
        }
    });

    const struct {
        DolphinView::Mode mode;
        const char *icon;
        QString toolTip;
    } modes[] = {
        {DolphinView::DetailsView, "view-list-details", i18nc("@info:tooltip", "Details")},
        {DolphinView::CompactView, "view-list-text", i18nc("@info:tooltip", "Compact")},
        {DolphinView::IconsView, "view-list-icons", i18nc("@info:tooltip", "Icons")},
        {DolphinView::ColumnsView, "view-file-columns", i18nc("@info:tooltip", "Columns")},
    };
    for (const auto &m : modes) {
        QToolButton *button = createButton(this, QString::fromLatin1(m.icon), m.toolTip);
        button->setCheckable(true);
        button->setProperty("viewMode", int(m.mode));
        connect(button, &QToolButton::clicked, this, [this, mode = m.mode]() {
            Q_EMIT viewModeRequested(mode);
            updateModes();
        });
        row->addWidget(button);
        m_modeButtons << button;
    }
    row->addSpacing(8);
    row->addStretch();

    // Every button sorts by one role. A second click on the button of the current role reverses the order.
    const struct {
        const char *role;
        QString label;
    } roles[] = {
        {"text", i18nc("@action:button sort by", "Name")},
        {"type", i18nc("@action:button sort by", "Type")},
        {"modificationtime", i18nc("@action:button sort by", "Date")},
        {"size", i18nc("@action:button sort by", "Size")},
    };
    for (const auto &r : roles) {
        auto *button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setLayoutDirection(Qt::RightToLeft);
        button->setText(r.label);
        button->setCheckable(true);
        button->setToolTip(i18nc("@info:tooltip", "Sort by %1", r.label));
        connect(button, &QToolButton::clicked, this, [this, role = QByteArray(r.role)]() {
            sortBy(role);
        });
        row->addWidget(button);
        m_sortButtons.insert(r.role, button);
    }

    connectView();
}

void ViewHeader::setView(DolphinView *view)
{
    if (view == m_view) {
        return;
    }
    if (m_view) {
        disconnect(m_view, nullptr, this, nullptr);
    }
    m_view = view;
    connectView();
}

void ViewHeader::connectView()
{
    connect(m_view, &DolphinView::urlChanged, this, &ViewHeader::updateNavigation);
    connect(m_view, &DolphinView::urlChanged, this, &ViewHeader::updateTitle);
    connect(m_view, &DolphinView::itemCountChanged, this, &ViewHeader::updateCounts);
    connect(m_view, &DolphinView::selectionChanged, this, &ViewHeader::updateCounts);
    connect(m_view, &DolphinView::modeChanged, this, &ViewHeader::updateModes);
    connect(m_view, &DolphinView::sortRoleChanged, this, &ViewHeader::updateSorting);
    connect(m_view, &DolphinView::sortOrderChanged, this, &ViewHeader::updateSorting);

    updateTitle();
    updateCounts();
    updateNavigation();
    updateModes();
    updateSorting();
}

void ViewHeader::updateTitle()
{
    const QUrl url = m_view->url();
    QString name = url.adjusted(QUrl::StripTrailingSlash).fileName();
    if (name.isEmpty()) {
        name = url.isLocalFile() ? i18nc("@title the root folder", "Root") : url.host();
    }
    m_title->setText(name);
}

void ViewHeader::updateCounts()
{
    const int selected = m_view->selectedItemsCount();
    QString text = i18ncp("@info", "%1 item", "%1 items", m_view->itemsCount());
    if (selected > 0) {
        text += i18nc("@info", "  ·  %1 selected", selected);
    }
    m_counts->setText(text);
}

void ViewHeader::updateNavigation()
{
    m_back->setEnabled(m_navigator->historyIndex() < m_navigator->historySize() - 1);
    m_forward->setEnabled(m_navigator->historyIndex() > 0);
    const QUrl url = m_view->url().adjusted(QUrl::StripTrailingSlash);
    m_up->setEnabled(!url.path().isEmpty() && url.path() != QLatin1String("/"));
}

void ViewHeader::updateModes()
{
    // OneCommander's "Show Files View toolbar buttons".
    const bool buttonsShown = CommanderSettings::boolValue(QStringLiteral("ShowFilesViewToolbar"), true);
    for (QToolButton *button : std::as_const(m_modeButtons)) {
        button->setVisible(buttonsShown);
    }
    for (QToolButton *button : std::as_const(m_sortButtons)) {
        button->setVisible(buttonsShown);
    }
    for (QToolButton *button : std::as_const(m_modeButtons)) {
        button->setChecked(button->property("viewMode").toInt() == int(m_view->viewMode()));
    }
    // The details view has a header of its own, with the sorting over each column.
    for (QToolButton *button : std::as_const(m_sortButtons)) {
        button->setVisible(m_view->viewMode() != DolphinView::DetailsView);
    }
}

void ViewHeader::updateSorting()
{
    for (auto it = m_sortButtons.cbegin(); it != m_sortButtons.cend(); ++it) {
        const bool current = it.key() == m_view->sortRole();
        QToolButton *button = it.value();
        button->setChecked(current);
        const QString icon = m_view->sortOrder() == Qt::AscendingOrder ? QStringLiteral("arrow-up") : QStringLiteral("arrow-down");
        button->setIcon(current ? QIcon::fromTheme(icon) : QIcon());
    }
}

void ViewHeader::sortBy(const QByteArray &role)
{
    if (role == m_view->sortRole()) {
        m_view->setSortOrder(m_view->sortOrder() == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
    } else {
        m_view->setSortRole(role);
        m_view->setSortOrder(m_view->preferredSortOrder(role));
    }
}

void ViewHeader::setMenuHandler(std::function<void(QWidget *)> handler)
{
    s_menuHandler = std::move(handler);
}
