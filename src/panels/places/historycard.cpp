/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "historycard.h"

#include <KLocalizedString>

#include <QIcon>
#include <QMouseEvent>
#include <QPainter>

namespace
{
constexpr int LineHeight = 24;
constexpr int TitleHeight = 28;
constexpr int Margin = 8;
}

HistoryCard::HistoryCard(QWidget *parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void HistoryCard::setUrls(const QList<QUrl> &urls)
{
    const int before = int(m_urls.size());
    m_urls = urls.mid(0, MaxEntries);
    if (int(m_urls.size()) != before) {
        updateGeometry();
    }
    setVisible(!m_urls.isEmpty());
    update();
}

QList<QUrl> HistoryCard::urls() const
{
    return m_urls;
}

QSize HistoryCard::sizeHint() const
{
    return QSize(200, 2 * Margin + TitleHeight + LineHeight * int(m_urls.size()));
}

QRect HistoryCard::lineRect(int index) const
{
    return QRect(Margin * 2, Margin + TitleHeight + index * LineHeight, width() - Margin * 4, LineHeight);
}

void HistoryCard::paintEvent(QPaintEvent *)
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
    painter.drawText(QRect(Margin * 2, Margin, width() - Margin * 4, TitleHeight), Qt::AlignVCenter | Qt::AlignLeft, i18nc("@title:group", "History"));

    painter.setFont(font());
    const QIcon folder = QIcon::fromTheme(QStringLiteral("folder"));
    for (int i = 0; i < m_urls.size(); ++i) {
        const QRect line = lineRect(i);
        const QUrl &url = m_urls.at(i);
        const QString name =
            url.isLocalFile() ? (url.path() == QLatin1String("/") ? QStringLiteral("/") : url.fileName()) : url.toDisplayString(QUrl::PreferLocalFile);
        folder.paint(&painter, QRect(line.left(), line.top() + 4, 16, 16));
        painter.setPen(colors.color(QPalette::WindowText));
        painter.drawText(QRect(line.left() + 22, line.top(), line.width() - 22, line.height()),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         painter.fontMetrics().elidedText(name, Qt::ElideMiddle, line.width() - 22));
    }
}

void HistoryCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        for (int i = 0; i < m_urls.size(); ++i) {
            if (lineRect(i).contains(event->position().toPoint())) {
                Q_EMIT urlActivated(m_urls.at(i));
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}
