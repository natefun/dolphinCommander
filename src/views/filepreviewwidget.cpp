/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "filepreviewwidget.h"

#include <KIO/PreviewJob>
#include <KLocalizedString>

#include <QFile>
#include <QFontDatabase>
#include <QLabel>
#include <QMimeType>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWheelEvent>

FilePreviewWidget::FilePreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    m_title = new QLabel(this);
    m_title->setWordWrap(true);
    layout->addWidget(m_title);
    m_scroll = new QScrollArea(this);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setAlignment(Qt::AlignCenter);
    m_image = new QLabel(m_scroll);
    m_image->setAlignment(Qt::AlignCenter);
    m_scroll->setWidget(m_image);
    m_scroll->viewport()->installEventFilter(this);
    layout->addWidget(m_scroll);
    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_text);
    layout->addStretch();
    m_scroll->hide();
    m_text->hide();
}

QSize FilePreviewWidget::sizeHint() const
{
    return QSize(260, 400);
}

QString FilePreviewWidget::shownName() const
{
    return m_shownName;
}

void FilePreviewWidget::setItem(const KFileItem &item)
{
    ++m_generation;
    m_image->clear();
    m_pixmap = QPixmap();
    m_zoom = 1;
    m_text->clear();
    m_scroll->hide();
    m_text->hide();
    if (item.isNull() || item.isDir()) {
        m_shownName.clear();
        m_title->clear();
        return;
    }
    m_shownName = item.name();
    m_title->setText(item.name());

    const QString path = item.localPath();
    const QMimeType mime = item.determineMimeType();
    const QString noPreview = i18nc("@info", "%1 — no preview for this kind of file.", item.name());
    if (!path.isEmpty() && mime.inherits(QStringLiteral("text/plain"))) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            m_text->setPlainText(QString::fromUtf8(file.read(64 * 1024)));
            m_text->show();
        }
    } else if (!path.isEmpty() || mime.name().startsWith(QLatin1String("image/"))) {
        auto *job = new KIO::PreviewJob(KFileItemList{item}, QSize(600, 400));
        QPointer<FilePreviewWidget> self(this);
        const quint64 generation = m_generation;
        connect(job, &KIO::PreviewJob::gotPreview, this, [self, generation](const KFileItem &, const QPixmap &pixmap) {
            // An answer for a file that is no longer the one shown is dropped.
            if (self && self->m_generation == generation) {
                self->m_pixmap = pixmap;
                self->showImage();
                self->m_scroll->show();
            }
        });
        connect(job, &KIO::PreviewJob::failed, this, [self, generation, noPreview]() {
            if (self && self->m_generation == generation) {
                self->m_title->setText(noPreview);
            }
        });
        job->start();
    } else {
        m_title->setText(noPreview);
    }
}

qreal FilePreviewWidget::zoom() const
{
    return m_zoom;
}

void FilePreviewWidget::showImage()
{
    if (m_pixmap.isNull()) {
        return;
    }
    const int fitted = qMin(m_pixmap.width(), qMax(width() - 12, 64));
    m_image->setPixmap(m_pixmap.scaledToWidth(qMax(int(fitted * m_zoom), 8), Qt::SmoothTransformation));
    m_image->adjustSize();
}

bool FilePreviewWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_scroll->viewport() && !m_pixmap.isNull()) {
        switch (event->type()) {
        case QEvent::Wheel: {
            const int delta = static_cast<QWheelEvent *>(event)->angleDelta().y();
            if (delta != 0) {
                m_zoom = qBound(1.0, m_zoom * (delta > 0 ? 1.25 : 0.8), 16.0);
                showImage();
            }
            return true;
        }
        case QEvent::MouseButtonPress:
            m_dragFrom = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
            return true;
        case QEvent::MouseMove: {
            const QPoint now = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
            const QPoint moved = now - m_dragFrom;
            m_dragFrom = now;
            m_scroll->horizontalScrollBar()->setValue(m_scroll->horizontalScrollBar()->value() - moved.x());
            m_scroll->verticalScrollBar()->setValue(m_scroll->verticalScrollBar()->value() - moved.y());
            return true;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}
