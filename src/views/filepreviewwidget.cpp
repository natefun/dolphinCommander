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
#include <QPlainTextEdit>
#include <QPointer>
#include <QVBoxLayout>

FilePreviewWidget::FilePreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    m_title = new QLabel(this);
    m_title->setWordWrap(true);
    layout->addWidget(m_title);
    m_image = new QLabel(this);
    m_image->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_image);
    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_text);
    layout->addStretch();
    m_image->hide();
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
    m_text->clear();
    m_image->hide();
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
                self->m_image->setPixmap(pixmap.scaledToWidth(qMin(pixmap.width(), qMax(self->width() - 12, 64)), Qt::SmoothTransformation));
                self->m_image->show();
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
