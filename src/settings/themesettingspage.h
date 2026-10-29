/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef THEMESETTINGSPAGE_H
#define THEMESETTINGSPAGE_H

#include "settingspagebase.h"

class QListView;

/**
 * @brief Page for the color scheme of the window.
 *
 * It lists the color schemes that are available, and the chosen one is used when the settings are applied.
 */
class ThemeSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit ThemeSettingsPage(QWidget *parent);
    ~ThemeSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    /** Selects the color scheme that is in use at the moment. */
    void selectActiveScheme();

    QListView *m_listView = nullptr;
};

#endif
