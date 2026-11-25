/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWHEADER_H
#define VIEWHEADER_H

#include "dolphin_export.h"
#include "dolphinview.h"

#include <QByteArray>
#include <QHash>
#include <QPointer>
#include <QWidget>

#include <functional>

class DolphinUrlNavigator;
class QLabel;
class QToolButton;

/**
 * @brief The header of a view: the name of the folder, how many items it has, back, forward and up
 * buttons for this view only, the buttons for the view mode, and a button for every way of sorting.
 */
class DOLPHIN_EXPORT ViewHeader : public QWidget
{
    Q_OBJECT

public:
    /** @p navigator keeps the history of @p view and must live as long as the header. */
    ViewHeader(DolphinView *view, DolphinUrlNavigator *navigator, QWidget *parent = nullptr);

    /** Follows @p view from now on, in place of the view it had. The container replaces its view to switch to the columns view and back. */
    void setView(DolphinView *view);

    /** Called with the menu button when it is clicked. The main window shows its menus there, as OneCommander has no menu bar. */
    static void setMenuHandler(std::function<void(QWidget *button)> handler);

Q_SIGNALS:
    /** Emitted when a button of a view mode is clicked. The container sets the mode, as it may have to replace the view for it. */
    void viewModeRequested(DolphinView::Mode mode);

private:
    void connectView();
    void updateTitle();
    void updateCounts();
    void updateNavigation();
    void updateModes();
    void updateSorting();
    void sortBy(const QByteArray &role);

private:
    DolphinView *m_view;
    DolphinUrlNavigator *m_navigator;
    QLabel *m_title = nullptr;
    QLabel *m_counts = nullptr;
    QToolButton *m_back = nullptr;
    QToolButton *m_forward = nullptr;
    QToolButton *m_up = nullptr;
    QList<QToolButton *> m_modeButtons;
    QHash<QByteArray, QToolButton *> m_sortButtons;
};

#endif
