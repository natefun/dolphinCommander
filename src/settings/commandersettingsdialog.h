/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COMMANDERSETTINGSDIALOG_H
#define COMMANDERSETTINGSDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>
#include <QVariant>

class QTabWidget;

/**
 * @brief The settings window of dolphinCommander, laid out as OneCommander's: the tabs General, View, Columns, Preview,
 * Theme, Window, Advanced and Other, each with its groups of settings, with the wording of OneCommander.
 *
 * Every change is applied at once. A setting that has no meaning outside of Windows (the registry, Mica, Explorer) is shown
 * and cannot be changed.
 */
class CommanderSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Kind {
        Bool,
        Int,
        Choice,
        Text,
        Button,
    };
    enum class Support {
        /** The program does what the setting says. */
        Live,
        /** The setting is for Windows: it is shown, and cannot be changed. */
        WindowsOnly,
        /** The setting is kept, but the program does not act on it yet. */
        NotYet,
    };
    struct Choice {
        QString value;
        QString label;
    };
    struct Setting {
        QString tab;
        QString header;
        QString key;
        QString label;
        QString info;
        Kind kind = Kind::Bool;
        QVariant defaultValue;
        Support support = Support::Live;
        int minimum = 0;
        int maximum = 100;
        QString suffix;
        QList<Choice> choices;
    };

    explicit CommanderSettingsDialog(QWidget *parent = nullptr);

    /** All the settings of the window, in the order they are shown. */
    static const QList<Setting> &settings();
    /** The tabs, in their order. */
    static QStringList tabs();

private:
    void build();
    void restoreDefaults();
    void clearThumbnailCache();

    QTabWidget *m_tabs = nullptr;
};

#endif
