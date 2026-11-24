/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "itemdetailspane.h"

#include <KActionCollection>
#include <KFormat>
#include <KIO/PreviewJob>
#include <KIconLoader>
#include <KLocalizedString>
#include <KXmlGuiWindow>

#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QPointer>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
constexpr int PictureSize = 72;
const char *const Keys[] = {"Name", "Type", "Created", "Modified", "Age", "Size"};
}

ItemDetailsPane::ItemDetailsPane(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(6, 2, 6, 2);
    m_picture = new QLabel(this);
    m_picture->setFixedSize(PictureSize, PictureSize);
    m_picture->setAlignment(Qt::AlignCenter);
    outer->addWidget(m_picture);

    auto *text = new QVBoxLayout;
    text->setSpacing(0);
    m_summary = new QLabel(this);
    QFont bold = m_summary->font();
    bold.setBold(true);
    m_summary->setFont(bold);
    // The summary line has OneCommander's buttons beside it: cut, copy, rename and delete the selection.
    auto *summaryRow = new QHBoxLayout;
    summaryRow->setContentsMargins(0, 0, 0, 0);
    summaryRow->addWidget(m_summary, 1);
    const struct {
        const char *action;
        const char *icon;
        const char *tip;
    } buttons[] = {{"edit_cut", "edit-cut", "Cut"},
                   {"edit_copy", "edit-copy", "Copy"},
                   {"renamefile", "edit-rename", "Rename"},
                   {"movetotrash", "edit-delete", "Move to Trash"}};
    for (const auto &entry : buttons) {
        auto *button = new QToolButton(this);
        button->setObjectName(QString::fromLatin1(entry.action));
        button->setAutoRaise(true);
        button->setIcon(QIcon::fromTheme(QString::fromLatin1(entry.icon)));
        button->setIconSize(QSize(14, 14));
        button->setToolTip(i18nc("@info:tooltip", entry.tip));
        const QString name = QString::fromLatin1(entry.action);
        connect(button, &QToolButton::clicked, this, [this, name]() {
            // The action of the main window that has this name does the work, on the selection of the side that is active.
            if (auto *window = qobject_cast<KXmlGuiWindow *>(this->window())) {
                if (QAction *action = window->actionCollection()->action(name)) {
                    action->trigger();
                }
            }
        });
        summaryRow->addWidget(button);
    }
    text->addLayout(summaryRow);
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(0);
    int row = 0;
    for (const char *key : Keys) {
        auto *name = new QLabel(i18nc("@label details pane row", key), this);
        name->setEnabled(false);
        auto *value = new QLabel(this);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setMinimumWidth(10);
        // Two columns of three rows keep the pane low.
        grid->addWidget(name, row % 3, (row / 3) * 2, Qt::AlignTop);
        grid->addWidget(value, row % 3, (row / 3) * 2 + 1);
        m_rows.append(qMakePair(name, value));
        ++row;
    }
    grid->setColumnStretch(1, 3);
    grid->setColumnStretch(3, 2);
    grid->setColumnMinimumWidth(2, 12);
    text->addLayout(grid);
    outer->addLayout(text, 1);
}

QSize ItemDetailsPane::sizeHint() const
{
    return QSize(300, PictureSize + 8);
}

QString ItemDetailsPane::ageText(const QDateTime &time, const QDateTime &now)
{
    if (!time.isValid()) {
        return QString();
    }
    const qint64 days = qMax<qint64>(time.daysTo(now), 0);
    if (days < 45) {
        return i18nc("@item:intext age in days", "%1 d", days);
    }
    if (days < 730) {
        return i18nc("@item:intext age in months", "%1 M", days / 30);
    }
    return i18nc("@item:intext age in years", "%1 y", days / 365);
}

QString ItemDetailsPane::summaryText() const
{
    return m_summary->text();
}

QString ItemDetailsPane::valueOf(const QString &key) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (QString::fromLatin1(Keys[i]) == key) {
            return m_rows.at(i).second->text();
        }
    }
    return QString();
}

void ItemDetailsPane::setContents(const KFileItem &folder, int itemCount, const KFileItemList &selection)
{
    ++m_generation;
    const KFormat format;
    QString summary = i18ncp("@info", "%1 item", "%1 items", itemCount);
    if (!selection.isEmpty()) {
        KIO::filesize_t total = 0;
        for (const KFileItem &item : selection) {
            if (item.isFile()) {
                total += item.size();
            }
        }
        summary += i18nc("@info separates the counts", ", ") + i18ncp("@info", "%1 selected", "%1 selected", selection.count());
        if (total > 0) {
            summary += QLatin1String(", ") + format.formatByteSize(total);
        }
    }
    m_summary->setText(summary);

    // One selected item is described; several or none describe the folder.
    const KFileItem item = selection.count() == 1 ? selection.first() : folder;
    QStringList values;
    if (item.isNull()) {
        for (const auto &row : std::as_const(m_rows)) {
            row.second->clear();
        }
        m_picture->clear();
        return;
    }
    const QLocale locale;
    const QDateTime modified = item.time(KFileItem::ModificationTime);
    const QDateTime created = item.time(KFileItem::CreationTime);
    values = {item.name(),
              item.mimeComment(),
              created.isValid() ? locale.toString(created, QLocale::ShortFormat) : QString(),
              modified.isValid() ? locale.toString(modified, QLocale::ShortFormat) : QString(),
              ageText(modified),
              item.isDir() ? QString() : format.formatByteSize(item.size())};
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows.at(i).second->setText(values.at(i));
        m_rows.at(i).second->setToolTip(values.at(i));
    }

    // A file gets its thumbnail; until it comes, and for everything without one, the icon of its kind.
    m_picture->setPixmap(QIcon::fromTheme(item.iconName()).pixmap(QSize(PictureSize / 2, PictureSize / 2), devicePixelRatioF()));
    if (item.isFile() && !selection.isEmpty()) {
        auto *job = new KIO::PreviewJob(KFileItemList{item}, QSize(PictureSize, PictureSize) * devicePixelRatioF());
        QPointer<ItemDetailsPane> self(this);
        const quint64 generation = m_generation;
        connect(job, &KIO::PreviewJob::gotPreview, this, [self, generation](const KFileItem &, const QPixmap &pixmap) {
            if (self && self->m_generation == generation) {
                self->m_picture->setPixmap(
                    pixmap.scaled(QSize(PictureSize, PictureSize) * self->devicePixelRatioF(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        });
        job->start();
    }
}
