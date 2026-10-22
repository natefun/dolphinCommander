/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef RIBBONBAR_H
#define RIBBONBAR_H

#include <QPointer>
#include <QWidget>

class QAction;
class QHBoxLayout;
class QMenu;
class QMenuBar;
class QScrollArea;
class QTabBar;

/**
 * @brief A ribbon built from the menus of a menu bar.
 *
 * Every top-level menu of the menu bar becomes a tab. The page of a tab shows the actions of
 * that menu as buttons next to each other, submenus as buttons that open the submenu. The
 * page is rebuilt every time it is shown, so menus that fill themselves when they are about
 * to be shown are always up to date.
 */
class RibbonBar : public QWidget
{
    Q_OBJECT

public:
    explicit RibbonBar(QMenuBar *source, QWidget *parent = nullptr);

    /** Shows @p actions as small buttons in front of the tabs, in the row of the tab bar. */
    void setLeadingActions(const QList<QAction *> &actions);

private:
    void showPage(int index);

private:
    QPointer<QMenuBar> m_source;
    QHBoxLayout *m_tabRowLayout = nullptr;
    QTabBar *m_tabBar = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QList<QPointer<QMenu>> m_menus;
};

#endif
