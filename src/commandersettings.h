/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COMMANDERSETTINGS_H
#define COMMANDERSETTINGS_H

#include "dolphin_export.h"

#include <QObject>
#include <QString>
#include <QVariant>

/**
 * The settings of dolphinCommander's own settings window, which has OneCommander's tabs and settings.
 * They are kept in the "Commander" group of the configuration, next to the settings the menus already keep there,
 * so that a menu entry and its setting in the window are the same thing.
 */
namespace CommanderSettings
{
DOLPHIN_EXPORT QVariant value(const QString &key, const QVariant &defaultValue = QVariant());
/** True if the setting was ever changed, so that its default is not what is in effect. */
DOLPHIN_EXPORT bool has(const QString &key);
DOLPHIN_EXPORT bool boolValue(const QString &key, bool defaultValue = false);
DOLPHIN_EXPORT int intValue(const QString &key, int defaultValue = 0);
DOLPHIN_EXPORT QString stringValue(const QString &key, const QString &defaultValue = QString());
/** Saves @p newValue and tells everyone who listens to notifier(). Nothing happens if it is the value that is saved already. */
DOLPHIN_EXPORT void setValue(const QString &key, const QVariant &newValue);
DOLPHIN_EXPORT void remove(const QString &key);

/** Emits changed() for every setting that is set with setValue(). */
class DOLPHIN_EXPORT Notifier : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void changed(const QString &key);
};
DOLPHIN_EXPORT Notifier *notifier();
}

#endif
