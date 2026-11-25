/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HISTORYCARD_H
#define HISTORYCARD_H

#include <QList>
#include <QUrl>
#include <QWidget>

/**
 * @brief OneCommander's History card: a rounded card under the places with the folders that were opened last,
 * the newest first. A click on a line opens that folder.
 */
class HistoryCard : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryCard(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    /** The folders shown: the first @c MaxEntries of @p urls. */
    void setUrls(const QList<QUrl> &urls);
    QList<QUrl> urls() const;

    static constexpr int MaxEntries = 6;

Q_SIGNALS:
    void urlActivated(const QUrl &url);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QRect lineRect(int index) const;

    QList<QUrl> m_urls;
};

#endif
