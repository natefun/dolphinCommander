/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FOLDERCARD_H
#define FOLDERCARD_H

#include <QList>
#include <QUrl>
#include <QWidget>

/**
 * @brief One of OneCommander's cards: a rounded card with a title and a list of folders, used for History (the folders opened last,
 * newest first) and for every favorite group. A click on a line opens that folder. The "+" button, when shown, asks to add the current folder.
 */
class FolderCard : public QWidget
{
    Q_OBJECT

public:
    explicit FolderCard(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    void setTitle(const QString &title);
    QString title() const;
    /** Shows the "+" button at the right of the title. */
    void setPlusShown(bool shown);
    void setMaxEntries(int count);

    /** The folders shown: the first entries of @p urls, as many as the card allows. */
    void setUrls(const QList<QUrl> &urls);
    QList<QUrl> urls() const;

    static constexpr int MaxEntries = 6;

Q_SIGNALS:
    void urlActivated(const QUrl &url);
    void plusClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QRect lineRect(int index) const;

    QList<QUrl> m_urls;
    QString m_title;
    bool m_plusShown = false;
    int m_maxEntries = MaxEntries;
};

#endif
