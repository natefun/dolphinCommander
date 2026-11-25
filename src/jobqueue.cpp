/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "jobqueue.h"

#include <KConfigGroup>
#include <KIO/CopyJob>
#include <KJob>
#include <KSharedConfig>

#include <QList>
#include <QPointer>

namespace
{
bool s_enabled = true;
QPointer<KJob> s_running;
QList<QPointer<KJob>> s_waiting;

void startNext()
{
    while (!s_waiting.isEmpty()) {
        QPointer<KJob> next = s_waiting.takeFirst();
        if (next) {
            s_running = next;
            next->resume();
            return;
        }
    }
    s_running = nullptr;
}
}

void JobQueue::setEnabled(bool enabled)
{
    s_enabled = enabled;
}

bool JobQueue::isEnabled()
{
    return s_enabled;
}

int JobQueue::count()
{
    int count = s_running ? 1 : 0;
    for (const QPointer<KJob> &job : std::as_const(s_waiting)) {
        count += job ? 1 : 0;
    }
    return count;
}

QStringList JobQueue::recentDestinations()
{
    return KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("RecentDestinations", QStringList());
}

void JobQueue::rememberDestination(const QString &folder)
{
    QStringList folders = recentDestinations();
    folders.removeAll(folder);
    folders.prepend(folder);
    while (folders.size() > 10) {
        folders.removeLast();
    }
    KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("RecentDestinations", folders);
}

void JobQueue::add(KJob *job)
{
    if (auto *copy = qobject_cast<KIO::CopyJob *>(job)) {
        if (copy->destUrl().isLocalFile()) {
            rememberDestination(copy->destUrl().toLocalFile());
        }
    }
    if (!s_enabled || !job) {
        return;
    }
    QObject::connect(job, &KJob::finished, job, [job]() {
        if (s_running == job) {
            startNext();
        } else {
            s_waiting.removeAll(QPointer<KJob>(job));
        }
    });
    if (!s_running) {
        s_running = job;
        return;
    }
    s_waiting.append(job);
    job->suspend();
}
