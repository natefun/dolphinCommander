/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "drivescard.h"

#include <KLocalizedString>

#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QSet>
#include <QStorageInfo>
#include <QTimer>

namespace
{
constexpr int LineHeight = 30;
constexpr int TitleHeight = 28;
constexpr int Margin = 8;
}

DrivesCard::DrivesCard(QWidget *parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    m_timer = new QTimer(this);
    m_timer->setInterval(15000);
    connect(m_timer, &QTimer::timeout, this, &DrivesCard::refresh);
    refresh();
    m_timer->start();
}

QString DrivesCard::usageText(qint64 usedBytes, qint64 totalBytes)
{
    const QLocale locale;
    const double gb = 1000.0 * 1000.0 * 1000.0;
    return i18nc("@info used and total size of a drive in gigabytes",
                 "%1 / %2 GB",
                 locale.toString(qRound64(usedBytes / gb)),
                 locale.toString(qRound64(totalBytes / gb)));
}

QStringList DrivesCard::mountPoints() const
{
    QStringList paths;
    for (const Drive &drive : m_drives) {
        paths << drive.path;
    }
    return paths;
}

void DrivesCard::refresh()
{
    QList<Drive> drives;
    QSet<QByteArray> devices; // subvolumes and bind mounts of one disk show up once
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &volume : volumes) {
        if (!volume.isValid() || !volume.isReady() || volume.bytesTotal() <= 0) {
            continue;
        }
        const QString path = volume.rootPath();
        // Only real drives: the root, home and what is mounted by the user, not the system's own pseudo file systems.
        const QByteArray type = volume.fileSystemType();
        if (type == "tmpfs" || type == "devtmpfs" || type == "squashfs" || type == "overlay" || type.startsWith("fuse.portal")) {
            continue;
        }
        if (path.startsWith(QLatin1String("/boot")) || path.startsWith(QLatin1String("/sys")) || path.startsWith(QLatin1String("/proc"))
            || path.startsWith(QLatin1String("/run")) || path.startsWith(QLatin1String("/var/lib")) || path.startsWith(QLatin1String("/snap"))) {
            continue;
        }
        const QByteArray device = volume.device();
        if (!device.isEmpty() && devices.contains(device)) {
            continue;
        }
        devices.insert(device);
        Drive drive;
        drive.path = path;
        drive.name =
            path == QLatin1String("/") ? i18nc("@item the root folder as a drive", "Root") : (volume.name().isEmpty() ? volume.displayName() : volume.name());
        drive.total = volume.bytesTotal();
        drive.used = volume.bytesTotal() - volume.bytesAvailable();
        drives << drive;
    }
    const bool changed = drives.size() != m_drives.size();
    m_drives = drives;
    if (changed) {
        updateGeometry();
    }
    update();
}

QSize DrivesCard::sizeHint() const
{
    return QSize(200, 2 * Margin + TitleHeight + LineHeight * int(m_drives.size()));
}

void DrivesCard::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refresh();
}

QRect DrivesCard::lineRect(int index) const
{
    return QRect(Margin * 2, Margin + TitleHeight + index * LineHeight, width() - Margin * 4, LineHeight);
}

void DrivesCard::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QPalette &colors = palette();
    const QColor window = colors.color(QPalette::Window);
    const QColor card = window.lightness() < 128 ? window.lighter(125) : window.darker(104);
    painter.setPen(Qt::NoPen);
    painter.setBrush(card);
    painter.drawRoundedRect(rect().adjusted(Margin / 2, Margin / 2, -Margin / 2, -Margin / 2), 8, 8);

    QFont titleFont = font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
    painter.setFont(titleFont);
    painter.setPen(colors.color(QPalette::WindowText));
    painter.drawText(QRect(Margin * 2, Margin, width() - Margin * 4, TitleHeight), Qt::AlignVCenter | Qt::AlignLeft, i18nc("@title:group", "Drives"));

    painter.setFont(font());
    for (int i = 0; i < m_drives.size(); ++i) {
        const Drive &drive = m_drives.at(i);
        const QRect line = lineRect(i);
        const QRect top = QRect(line.left(), line.top(), line.width(), line.height() - 8);
        painter.setPen(colors.color(QPalette::WindowText));
        QFont bold = font();
        bold.setBold(true);
        painter.setFont(bold);
        const QString usage = usageText(drive.used, drive.total);
        painter.setFont(font());
        const int usageWidth = painter.fontMetrics().horizontalAdvance(usage);
        painter.drawText(top, Qt::AlignVCenter | Qt::AlignRight, usage);
        painter.setFont(bold);
        const QString name = painter.fontMetrics().elidedText(drive.name, Qt::ElideMiddle, qMax(20, top.width() - usageWidth - 8));
        painter.drawText(top, Qt::AlignVCenter | Qt::AlignLeft, name);
        painter.setFont(font());

        const QRect bar(line.left(), line.bottom() - 4, line.width(), 3);
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.color(QPalette::Mid));
        painter.drawRect(bar);
        const double fraction = drive.total > 0 ? double(drive.used) / double(drive.total) : 0.0;
        painter.setBrush(fraction > 0.9 ? QColor(200, 70, 60) : colors.color(QPalette::Highlight));
        painter.drawRect(QRect(bar.left(), bar.top(), int(bar.width() * qBound(0.0, fraction, 1.0)), bar.height()));
    }
}

void DrivesCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        for (int i = 0; i < m_drives.size(); ++i) {
            if (lineRect(i).contains(event->position().toPoint())) {
                Q_EMIT driveActivated(QUrl::fromLocalFile(m_drives.at(i).path));
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}
