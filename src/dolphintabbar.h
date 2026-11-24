/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_TAB_BAR_H
#define DOLPHIN_TAB_BAR_H

#include <QDateTime>
#include <QPointer>
#include <QTabBar>

class QToolButton;

class DolphinTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DolphinTabBar(QWidget *parent);

    bool isPinned(int index) const;
    void setPinned(int index, bool pinned);
    int pinnedCount() const;

    void setNewTabButtonVisible(bool visible);

    /**
     * The tabs that were not shown for @p seconds before @p now (a day by default), except the current one and the pinned ones: OneCommander's "stale tabs".
     * A tab counts from when it was made or last became the current one in this run.
     */
    QList<int> staleTabs(const QDateTime &now = QDateTime::currentDateTime(), qint64 seconds = 24 * 3600) const;
    /** Says when the tab at @p index was last shown; for the stale tabs, and for tests. */
    void setLastShown(int index, const QDateTime &time);

Q_SIGNALS:
    void openNewActivatedTab(int index);
    void tabDragMoveEvent(int index, QDragMoveEvent *event);
    void tabDropEvent(int index, QDropEvent *event);
    void tabDetachRequested(int index);
    /** Emitted when the tab at @p index was dropped on @p target, the tab bar of the other side, to go to @p targetIndex there (-1 for the end). */
    void tabMoveRequested(int index, DolphinTabBar *target, int targetIndex);
    /** Emitted when the tab at @p index was clicked with the middle button while Alt was held: it is to go to the other side. */
    void tabMoveToOtherSideRequested(int index);
    /** Emitted when the tab at @p index is to be copied to the other side: a new tab there with the same folder. */
    void tabCopyToOtherSideRequested(int index);
    void tabRenamed(int index, const QString &label);
    void newTabRequested();
    void tabPinStateChanged(int index, bool pinned);

protected:
    QSize tabSizeHint(int index) const override;
    QSize minimumSizeHint() const override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void tabInserted(int index) override;
    void tabRemoved(int index) override;
    void tabLayoutChange() override;

    /**
     * Opens a context menu for the tab on the \a event position.
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

    /** While a tab of this bar is dragged, takes the drag over the other side of the window. */
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void slotAutoActivationTimeout();
    void slotTabBarChanged();

private:
    void updateAutoActivationTimer(const int index);
    void updateNewTabButtonGeometry();
    void updatePinnedTabButtons();

    bool isOutsideUndetachableZone(const QPoint &pos) const;
    void updateTabDetachDrag(QMouseEvent *event);
    void startTabDrag(QMouseEvent *event);
    void cancelTabDrag();

    /** Returns the tab bar of the side of this window that @p widget is on, if that is not the side of this bar. */
    DolphinTabBar *otherSideTabBarAt(QWidget *widget) const;

private:
    struct DragState {
        int tabIndex = -1;
        QPoint startPosition;
        int offsetInTabX = 0;
        int offsetInTabY = 0;
        int tabWidth = 0;
        int tabHeight = 0;
        bool isOngoing = false;
    };

    QTimer *m_autoActivationTimer;
    int m_autoActivationIndex;
    int m_tabToBeClosedOnMiddleMouseButtonRelease = -1;
    QToolButton *m_newTabButton = nullptr;
    DragState m_drag;
    int m_pressedTab = -1;
    int m_dragBoundaryX = -1;
    int m_clickOffsetInTab = 0;

    /** Where the tab that is being dragged was dropped on the other side, if it was. */
    QPointer<DolphinTabBar> m_dropTarget;
    int m_dropTargetIndex = -1;
};

#endif // DOLPHIN_TAB_BAR_H
