/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWHEADER_H
#define VIEWHEADER_H

#include "dolphin_export.h"

#include <QByteArray>
#include <QHash>
#include <QWidget>

class DolphinUrlNavigator;
class DolphinView;
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

private:
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
