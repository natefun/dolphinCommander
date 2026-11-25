/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FAVORITESPANEL_H
#define FAVORITESPANEL_H

#include "dolphin_export.h"

#include <QTreeWidget>
#include <QUrl>

/**
 * OneCommander's favorites: folders that are kept in named groups. A folder is added with addFavorite() or by
 * dropping it on the panel, and a click opens it. The groups are kept in the settings of the program.
 */
class DOLPHIN_EXPORT FavoritesPanel : public QTreeWidget
{
    Q_OBJECT
public:
    explicit FavoritesPanel(QWidget *parent = nullptr);

    /** Adds @p url to @p group, which is made if it is new. A folder that is in the group already is not added twice. */
    void addFavorite(const QUrl &url, const QString &group);
    /** Makes an empty group. Does nothing if it exists. */
    void addGroup(const QString &group);
    void removeFavorite(const QUrl &url, const QString &group);
    void removeGroup(const QString &group);
    void renameGroup(const QString &oldName, const QString &newName);

    /** Notes that @p url was opened: it goes to the top of the History group, which keeps the last 20 folders, each once. */
    void addHistory(const QUrl &url);
    QList<QUrl> history() const;
    void clearHistory();
    /** Shows @p url under @p alias in the panel, without renaming the folder. An empty alias gives the folder's own name back. */
    void setAlias(const QUrl &url, const QString &alias);
    QString aliasOf(const QUrl &url) const;

    QStringList groups() const;
    QList<QUrl> favorites(const QString &group) const;

Q_SIGNALS:
    /** A favorite was clicked. */
    void urlActivated(const QUrl &url);
    /** A favorite was asked to open on the other side or in a new tab. */
    void urlActivatedInNewTab(const QUrl &url);
    /** The favorites, their groups or the aliases were changed. */
    void changed();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void load();
    void save();
    QTreeWidgetItem *groupItem(const QString &group, bool create);
    void populate(QTreeWidgetItem *group, const QList<QUrl> &urls);

    QTreeWidgetItem *historyItem();
    QString displayName(const QUrl &url) const;

    bool m_loading = false;
};

#endif
