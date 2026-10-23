/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "languagesettingspage.h"

#include <KLocalizedString>

#include <QAction>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

LanguageSettingsPage::LanguageSettingsPage(QAction *switchLanguageAction, QWidget *parent)
    : SettingsPageBase(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *label = new QLabel(
        i18nc(
            "@info",
            "The language of this application can be different from the language of the system. The change is used the next time the application is started."),
        this);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto *button = new QPushButton(QIcon::fromTheme(QStringLiteral("preferences-desktop-locale")), i18nc("@action:button", "Change Language…"), this);
    button->setEnabled(switchLanguageAction != nullptr);
    if (switchLanguageAction) {
        connect(button, &QPushButton::clicked, switchLanguageAction, &QAction::trigger);
    }
    layout->addWidget(button, 0, Qt::AlignLeft);
    layout->addStretch();
}

LanguageSettingsPage::~LanguageSettingsPage() = default;

void LanguageSettingsPage::applySettings()
{
    // The language is saved by the dialog that this page opens.
}

void LanguageSettingsPage::restoreDefaults()
{
}
