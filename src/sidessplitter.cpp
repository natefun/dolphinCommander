/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "sidessplitter.h"

#include <KLocalizedString>

#include <QIcon>
#include <QSplitterHandle>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
constexpr int HandleWidth = 26;
constexpr int ArrowSize = 18;
constexpr int ButtonHeight = 44;

class SidesSplitterHandle : public QSplitterHandle
{
public:
    SidesSplitterHandle(SidesSplitter *splitter)
        : QSplitterHandle(Qt::Horizontal, splitter)
        , m_splitter(splitter)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        layout->addStretch();
        m_leftButton = createButton(QStringLiteral("go-previous"), -1);
        m_rightButton = createButton(QStringLiteral("go-next"), 1);
        layout->addWidget(m_leftButton);
        layout->addWidget(m_rightButton);
        layout->addStretch();

        connect(splitter, &SidesSplitter::hiddenSidesChanged, this, [this]() {
            updateButtons();
        });
        updateButtons();
    }

private:
    QToolButton *createButton(const QString &iconName, int direction)
    {
        auto *button = new QToolButton(this);
        // A button with a face, so that it is easy to see and to hit.
        button->setAutoRaise(false);
        button->setFocusPolicy(Qt::NoFocus);
        button->setIcon(QIcon::fromTheme(iconName));
        button->setIconSize(QSize(ArrowSize, ArrowSize));
        button->setFixedSize(HandleWidth, ButtonHeight);
        // Without this the button shows the resize cursor of the handle it is on.
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QToolButton::clicked, this, [this, direction]() {
            m_splitter->moveDivider(direction);
        });
        return button;
    }

    /** An arrow is only there if the divider can still be moved that way. */
    void updateButtons()
    {
        const bool leftHidden = m_splitter->isSideHidden(0);
        const bool rightHidden = m_splitter->isSideHidden(1);
        m_leftButton->setVisible(!leftHidden);
        m_rightButton->setVisible(!rightHidden);
        m_leftButton->setToolTip(rightHidden ? i18nc("@info:tooltip", "Show both sides") : i18nc("@info:tooltip", "Hide the left side"));
        m_rightButton->setToolTip(leftHidden ? i18nc("@info:tooltip", "Show both sides") : i18nc("@info:tooltip", "Hide the right side"));
    }

    SidesSplitter *m_splitter;
    QToolButton *m_leftButton = nullptr;
    QToolButton *m_rightButton = nullptr;
};
}

SidesSplitter::SidesSplitter(QWidget *parent)
    : QSplitter(Qt::Horizontal, parent)
{
    setHandleWidth(HandleWidth);
    setChildrenCollapsible(true);
    // The handle is only created by QSplitter when a widget is added. Whether a side is hidden
    // can change by dragging the handle too, so this is where all changes are noticed.
    connect(this, &QSplitter::splitterMoved, this, &SidesSplitter::updateHiddenSides);
}

QSplitterHandle *SidesSplitter::createHandle()
{
    return new SidesSplitterHandle(this);
}

bool SidesSplitter::isSideHidden(int index) const
{
    const QList<int> widths = sizes();
    if (index < 0 || index >= 2 || widths.size() < 2) {
        return false;
    }
    // Before the first layout all sizes are 0, which does not mean that a side is hidden.
    if (widths.at(0) + widths.at(1) <= 0) {
        return false;
    }
    return widths.at(index) == 0;
}

void SidesSplitter::updateHiddenSides()
{
    const bool leftHidden = isSideHidden(0);
    const bool rightHidden = isSideHidden(1);
    if (leftHidden != m_leftWasHidden || rightHidden != m_rightWasHidden) {
        m_leftWasHidden = leftHidden;
        m_rightWasHidden = rightHidden;
        Q_EMIT hiddenSidesChanged();
    }
}

void SidesSplitter::resizeEvent(QResizeEvent *event)
{
    QSplitter::resizeEvent(event);
    updateHiddenSides();
}

void SidesSplitter::showEvent(QShowEvent *event)
{
    QSplitter::showEvent(event);
    updateHiddenSides();
}

void SidesSplitter::showBothSides()
{
    if (!isSideHidden(0) && !isSideHidden(1)) {
        return;
    }
    QList<int> widths = m_widthsBeforeHiding;
    if (widths.size() != 2 || widths.at(0) <= 0 || widths.at(1) <= 0) {
        const int half = (width() - handleWidth()) / 2;
        widths = {half, half};
    }
    setSizes(widths);
    m_leftWasHidden = false;
    m_rightWasHidden = false;
    Q_EMIT hiddenSidesChanged();
}

void SidesSplitter::moveDivider(int direction)
{
    const bool leftHidden = isSideHidden(0);
    const bool rightHidden = isSideHidden(1);

    if ((direction < 0 && rightHidden) || (direction > 0 && leftHidden)) {
        showBothSides();
        return;
    }
    if (leftHidden || rightHidden) {
        return;
    }

    m_widthsBeforeHiding = sizes();
    const int total = width() - handleWidth();
    setSizes(direction < 0 ? QList<int>{0, total} : QList<int>{total, 0});
    m_leftWasHidden = isSideHidden(0);
    m_rightWasHidden = isSideHidden(1);
    Q_EMIT hiddenSidesChanged();
}
