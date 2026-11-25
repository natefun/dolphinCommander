/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commandersettings.h"

#include <KConfigGroup>
#include <KSharedConfig>

namespace
{
KConfigGroup group()
{
    return KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander"));
}
}

QVariant CommanderSettings::value(const QString &key, const QVariant &defaultValue)
{
    return group().readEntry(key, defaultValue);
}

bool CommanderSettings::has(const QString &key)
{
    return group().hasKey(key);
}

bool CommanderSettings::boolValue(const QString &key, bool defaultValue)
{
    return group().readEntry(key, defaultValue);
}

int CommanderSettings::intValue(const QString &key, int defaultValue)
{
    return group().readEntry(key, defaultValue);
}

QString CommanderSettings::stringValue(const QString &key, const QString &defaultValue)
{
    return group().readEntry(key, defaultValue);
}

void CommanderSettings::setValue(const QString &key, const QVariant &newValue)
{
    KConfigGroup commander = group();
    if (commander.hasKey(key) && commander.readEntry(key, QVariant(newValue.metaType())) == newValue) {
        return;
    }
    commander.writeEntry(key, newValue);
    commander.sync();
    Q_EMIT notifier()->changed(key);
}

void CommanderSettings::remove(const QString &key)
{
    KConfigGroup commander = group();
    if (commander.hasKey(key)) {
        commander.deleteEntry(key);
        commander.sync();
        Q_EMIT notifier()->changed(key);
    }
}

CommanderSettings::Notifier *CommanderSettings::notifier()
{
    static Notifier instance;
    return &instance;
}
