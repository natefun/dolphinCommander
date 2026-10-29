/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "themesettingspage.h"

#include <KColorSchemeManager>
#include <KColorSchemeModel>
#include <KLocalizedString>

#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QVBoxLayout>

ThemeSettingsPage::ThemeSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *label = new QLabel(i18nc("@info", "The colors of the window. The first entry follows the theme of the system."), this);
    label->setWordWrap(true);
    layout->addWidget(label);

    m_listView = new QListView(this);
    m_listView->setModel(KColorSchemeManager::instance()->model());
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_listView);

    selectActiveScheme();
    connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ThemeSettingsPage::changed);
}

ThemeSettingsPage::~ThemeSettingsPage() = default;

void ThemeSettingsPage::selectActiveScheme()
{
    auto *manager = KColorSchemeManager::instance();
    m_listView->setCurrentIndex(manager->indexForSchemeId(manager->activeSchemeId()));
}

void ThemeSettingsPage::applySettings()
{
    const QModelIndex current = m_listView->currentIndex();
    if (current.isValid()) {
        KColorSchemeManager::instance()->activateSchemeId(current.data(KColorSchemeModel::IdRole).toString());
    }
}

void ThemeSettingsPage::restoreDefaults()
{
    // The entry that follows the theme of the system has an empty id.
    m_listView->setCurrentIndex(KColorSchemeManager::instance()->indexForSchemeId(QString()));
}
