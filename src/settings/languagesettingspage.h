/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LANGUAGESETTINGSPAGE_H
#define LANGUAGESETTINGSPAGE_H

#include "settingspagebase.h"

class QAction;

/**
 * @brief Page that leads to the choice of the language of the application.
 *
 * The choice itself is made in the dialog that KDE provides for it, which is opened by the
 * button of this page.
 */
class LanguageSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    /** @param switchLanguageAction The action that opens the dialog to choose the language. */
    LanguageSettingsPage(QAction *switchLanguageAction, QWidget *parent);
    ~LanguageSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;
};

#endif
