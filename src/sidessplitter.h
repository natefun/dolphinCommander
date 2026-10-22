/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SIDESSPLITTER_H
#define SIDESSPLITTER_H

#include <QList>
#include <QSplitter>

class QResizeEvent;
class QShowEvent;

/**
 * @brief The splitter between the two sides of the window.
 *
 * In the middle of its divider are two arrows that move the divider to the left or to the
 * right. Moving it all the way to one edge hides the side that is on that edge, the divider
 * stays there and moving it back shows the side again. Dragging the divider does the same.
 */
class SidesSplitter : public QSplitter
{
    Q_OBJECT

public:
    explicit SidesSplitter(QWidget *parent = nullptr);

    /** Returns whether the side at @p index (0 for the left, 1 for the right) is hidden. */
    bool isSideHidden(int index) const;

    /** Shows both sides, in the widths they had before one of them was hidden. */
    void showBothSides();

    /** Moves the divider one step: -1 to the left, +1 to the right. */
    void moveDivider(int direction);

Q_SIGNALS:
    /** Emitted when a side got hidden or shown. */
    void hiddenSidesChanged();

protected:
    QSplitterHandle *createHandle() override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    /** Emits hiddenSidesChanged() if a side got hidden or shown since the last time. */
    void updateHiddenSides();

    QList<int> m_widthsBeforeHiding;
    bool m_leftWasHidden = false;
    bool m_rightWasHidden = false;
};

#endif
