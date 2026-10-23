/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "shortcutssettingspage.h"

#include <KShortcutsEditor>

#include <QVBoxLayout>

ShortcutsSettingsPage::ShortcutsSettingsPage(KActionCollection *actions, QWidget *parent)
    : SettingsPageBase(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // A single letter as a shortcut would take the typing away from the filter of the file list.
    m_editor = new KShortcutsEditor(actions, this, KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsDisallowed);
    layout->addWidget(m_editor);

    connect(m_editor, &KShortcutsEditor::keyChange, this, &ShortcutsSettingsPage::changed);
}

ShortcutsSettingsPage::~ShortcutsSettingsPage() = default;

void ShortcutsSettingsPage::applySettings()
{
    m_editor->save();
}

void ShortcutsSettingsPage::restoreDefaults()
{
    m_editor->allDefault();
}
