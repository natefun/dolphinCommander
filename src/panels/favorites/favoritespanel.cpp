/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "favoritespanel.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QIcon>
#include <QInputDialog>
#include <QMenu>
#include <QMimeData>

namespace
{
constexpr int UrlRole = Qt::UserRole;

KConfigGroup settings()
{
    return KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("CommanderFavorites"));
}

QString nameOf(const QUrl &url)
{
    const QString name = url.fileName();
    return name.isEmpty() ? url.toDisplayString(QUrl::PreferLocalFile) : name;
}
}

FavoritesPanel::FavoritesPanel(QWidget *parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setEditTriggers(NoEditTriggers);
    connect(this, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item) {
        const QUrl url = item->data(0, UrlRole).toUrl();
        if (url.isValid()) {
            Q_EMIT urlActivated(url);
        }
    });
    load();
}

void FavoritesPanel::load()
{
    m_loading = true;
    clear();
    const KConfigGroup group = settings();
    const QStringList names = group.readEntry("Groups", QStringList());
    for (const QString &name : names) {
        QList<QUrl> urls;
        for (const QString &text : group.readEntry("Group " + name, QStringList())) {
            urls << QUrl::fromUserInput(text);
        }
        populate(groupItem(name, true), urls);
    }
    expandAll();
    m_loading = false;
}

void FavoritesPanel::save() const
{
    if (m_loading) {
        return;
    }
    KConfigGroup group = settings();
    for (const QString &key : group.keyList()) {
        group.deleteEntry(key);
    }
    group.writeEntry("Groups", groups());
    for (const QString &name : groups()) {
        QStringList urls;
        for (const QUrl &url : favorites(name)) {
            urls << url.toString();
        }
        group.writeEntry("Group " + name, urls);
    }
    group.sync();
}

QTreeWidgetItem *FavoritesPanel::groupItem(const QString &group, bool create)
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        if (topLevelItem(i)->text(0) == group) {
            return topLevelItem(i);
        }
    }
    if (!create) {
        return nullptr;
    }
    auto *item = new QTreeWidgetItem(this, {group});
    item->setIcon(0, QIcon::fromTheme(QStringLiteral("folder-favorites")));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);
    return item;
}

void FavoritesPanel::populate(QTreeWidgetItem *group, const QList<QUrl> &urls)
{
    for (const QUrl &url : urls) {
        auto *item = new QTreeWidgetItem(group, {nameOf(url)});
        item->setData(0, UrlRole, url);
        item->setToolTip(0, url.toDisplayString(QUrl::PreferLocalFile));
        item->setIcon(0, QIcon::fromTheme(QStringLiteral("folder")));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
}

void FavoritesPanel::addGroup(const QString &group)
{
    if (group.isEmpty() || groupItem(group, false)) {
        return;
    }
    groupItem(group, true)->setExpanded(true);
    save();
}

void FavoritesPanel::addFavorite(const QUrl &url, const QString &group)
{
    if (!url.isValid() || group.isEmpty()) {
        return;
    }
    if (favorites(group).contains(url)) {
        return;
    }
    QTreeWidgetItem *item = groupItem(group, true);
    populate(item, {url});
    item->setExpanded(true);
    save();
}

void FavoritesPanel::removeFavorite(const QUrl &url, const QString &group)
{
    QTreeWidgetItem *item = groupItem(group, false);
    if (!item) {
        return;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        if (item->child(i)->data(0, UrlRole).toUrl() == url) {
            delete item->takeChild(i);
            break;
        }
    }
    save();
}

void FavoritesPanel::removeGroup(const QString &group)
{
    delete groupItem(group, false);
    save();
}

void FavoritesPanel::renameGroup(const QString &oldName, const QString &newName)
{
    QTreeWidgetItem *item = groupItem(oldName, false);
    if (item && !newName.isEmpty() && !groupItem(newName, false)) {
        item->setText(0, newName);
        save();
    }
}

QStringList FavoritesPanel::groups() const
{
    QStringList names;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        names << topLevelItem(i)->text(0);
    }
    return names;
}

QList<QUrl> FavoritesPanel::favorites(const QString &group) const
{
    QList<QUrl> urls;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        if (topLevelItem(i)->text(0) == group) {
            for (int j = 0; j < topLevelItem(i)->childCount(); ++j) {
                urls << topLevelItem(i)->child(j)->data(0, UrlRole).toUrl();
            }
        }
    }
    return urls;
}

void FavoritesPanel::contextMenuEvent(QContextMenuEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());
    QMenu menu(this);
    QTreeWidgetItem *group = item ? (item->parent() ? item->parent() : item) : nullptr;
    if (item && item->parent()) {
        const QUrl url = item->data(0, UrlRole).toUrl();
        menu.addAction(QIcon::fromTheme(QStringLiteral("tab-new")), i18nc("@action:inmenu", "Open in New Tab"), this, [this, url]() {
            Q_EMIT urlActivatedInNewTab(url);
        });
        menu.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18nc("@action:inmenu", "Remove from Favorites"), this, [this, url, group]() {
            removeFavorite(url, group->text(0));
        });
        menu.addSeparator();
    } else if (group) {
        const QString name = group->text(0);
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18nc("@action:inmenu", "Rename Group…"), this, [this, name]() {
            bool ok = false;
            const QString text =
                QInputDialog::getText(this, i18nc("@title:window", "Rename Group"), i18nc("@label:textbox", "Name:"), QLineEdit::Normal, name, &ok);
            if (ok) {
                renameGroup(name, text.trimmed());
            }
        });
        menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action:inmenu", "Remove Group"), this, [this, name]() {
            removeGroup(name);
        });
        menu.addSeparator();
    }
    menu.addAction(QIcon::fromTheme(QStringLiteral("folder-new")), i18nc("@action:inmenu", "New Group…"), this, [this]() {
        bool ok = false;
        const QString text =
            QInputDialog::getText(this, i18nc("@title:window", "New Group"), i18nc("@label:textbox", "Name:"), QLineEdit::Normal, QString(), &ok);
        if (ok) {
            addGroup(text.trimmed());
        }
    });
    menu.exec(event->globalPos());
}

void FavoritesPanel::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void FavoritesPanel::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void FavoritesPanel::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        return;
    }
    // A folder dropped on a group goes into it; dropped on the panel it goes into the first group, which is made if there is none.
    QTreeWidgetItem *target = itemAt(event->position().toPoint());
    if (target && target->parent()) {
        target = target->parent();
    }
    const QString group = target ? target->text(0) : (topLevelItemCount() > 0 ? topLevelItem(0)->text(0) : i18nc("@item default group", "Favorites"));
    for (const QUrl &url : event->mimeData()->urls()) {
        addFavorite(url, group);
    }
    event->acceptProposedAction();
}
