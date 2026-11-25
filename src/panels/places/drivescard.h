/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DRIVESCARD_H
#define DRIVESCARD_H

#include <QList>
#include <QUrl>
#include <QWidget>

class QTimer;

/**
 * @brief OneCommander's Drives card: a rounded card above the places with one line per drive,
 * its mount point, a bar for how full it is and "used / total GB". A click on a line opens the drive.
 */
class DrivesCard : public QWidget
{
    Q_OBJECT

public:
    explicit DrivesCard(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    /** The mount points shown, in order. */
    QStringList mountPoints() const;

    /** "1,204 / 1,991 GB" for the used and total bytes. */
    static QString usageText(qint64 usedBytes, qint64 totalBytes);

Q_SIGNALS:
    void driveActivated(const QUrl &url);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    struct Drive {
        QString path;
        QString name;
        qint64 total = 0;
        qint64 used = 0;
    };
    void refresh();
    QRect lineRect(int index) const;

    QList<Drive> m_drives;
    QTimer *m_timer = nullptr;
};

#endif
