/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SHORTCUTSSETTINGSPAGE_H
#define SHORTCUTSSETTINGSPAGE_H

#include "settingspagebase.h"

class KActionCollection;
class KShortcutsEditor;

/**
 * @brief Page for the keyboard shortcuts of all actions.
 *
 * It holds the shortcuts editor that is otherwise shown in its own dialog.
 */
class ShortcutsSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    ShortcutsSettingsPage(KActionCollection *actions, QWidget *parent);
    ~ShortcutsSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    KShortcutsEditor *m_editor = nullptr;
};

#endif
