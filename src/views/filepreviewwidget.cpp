/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "filepreviewwidget.h"

#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <KZip>

#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QImageReader>
#include <QLabel>
#include <QMimeType>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QXmlStreamReader>

QString FilePreviewWidget::officeDocumentText(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    // The XML parts that hold the text, the element that ends a paragraph or a cell, and the element that holds text.
    struct Format {
        QString part;
        QString paragraph;
        QString text;
    };
    QList<Format> formats;
    if (suffix == QLatin1String("docx")) {
        formats = {{QStringLiteral("word/document.xml"), QStringLiteral("p"), QStringLiteral("t")}};
    } else if (suffix == QLatin1String("xlsx")) {
        formats = {{QStringLiteral("xl/sharedStrings.xml"), QStringLiteral("si"), QStringLiteral("t")}};
    } else if (suffix == QLatin1String("odt") || suffix == QLatin1String("ods") || suffix == QLatin1String("odp")) {
        formats = {{QStringLiteral("content.xml"), QStringLiteral("p"), QString()}};
    } else {
        return QString();
    }
    KZip zip(path);
    if (!zip.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QString result;
    for (const Format &format : std::as_const(formats)) {
        const KArchiveEntry *entry = zip.directory()->entry(format.part);
        if (!entry || !entry->isFile()) {
            continue;
        }
        const QByteArray xml = static_cast<const KArchiveFile *>(entry)->data();
        QXmlStreamReader reader(xml);
        bool inText = format.text.isEmpty();
        while (!reader.atEnd() && result.size() < 64 * 1024) {
            reader.readNext();
            if (reader.isStartElement() && !format.text.isEmpty() && reader.name() == format.text) {
                inText = true;
            } else if (reader.isEndElement() && !format.text.isEmpty() && reader.name() == format.text) {
                inText = false;
            } else if (reader.isCharacters() && inText) {
                result += reader.text();
            } else if (reader.isEndElement() && (reader.name() == format.paragraph || reader.name() == QLatin1String("h"))) {
                result += QLatin1Char('\n');
            }
        }
    }
    return result.trimmed();
}

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
    const QString officeText = path.isEmpty() ? QString() : officeDocumentText(path);
    if (!officeText.isEmpty()) {
        // A Word, Excel or OpenDocument file shows its text.
        m_text->setPlainText(officeText);
        m_text->show();
    } else if (!path.isEmpty() && mime.inherits(QStringLiteral("text/plain"))) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            m_text->setPlainText(QString::fromUtf8(file.read(64 * 1024)));
            m_text->show();
        }
    } else if (!path.isEmpty() && mime.name().startsWith(QLatin1String("image/")) && QImageReader(path).canRead()) {
        // A picture on this computer is shown itself, so that it can be shown at its own size.
        QImageReader reader(path);
        reader.setAutoTransform(true);
        const QSize size = reader.size();
        if (size.isValid() && (size.width() > 8000 || size.height() > 8000)) {
            reader.setScaledSize(size.scaled(8000, 8000, Qt::KeepAspectRatio));
        }
        const QImage image = reader.read();
        if (!image.isNull()) {
            m_pixmap = QPixmap::fromImage(image);
            showImage();
            m_scroll->show();
        } else {
            m_title->setText(noPreview);
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

void FilePreviewWidget::setImageMode(ImageMode mode)
{
    m_imageMode = mode;
    m_zoom = 1;
    showImage();
}

FilePreviewWidget::ImageMode FilePreviewWidget::imageMode() const
{
    return m_imageMode;
}

bool FilePreviewWidget::showsImage() const
{
    return !m_pixmap.isNull();
}

QSize FilePreviewWidget::shownImageSize() const
{
    return m_image->pixmap().isNull() ? QSize() : m_image->pixmap().size();
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
    const QSize room = m_scroll->viewport()->size().expandedTo(QSize(64, 64));
    QPixmap shown;
    switch (m_imageMode) {
    case ImageMode::Fit: {
        const QSize fitted = m_pixmap.size().scaled(room * m_zoom, Qt::KeepAspectRatio);
        shown = m_pixmap.scaled(fitted.expandedTo(QSize(8, 8)), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        break;
    }
    case ImageMode::FillWidth:
        shown = m_pixmap.scaledToWidth(qMax(int(room.width() * m_zoom), 8), Qt::SmoothTransformation);
        break;
    case ImageMode::ActualSize:
        shown = qFuzzyCompare(m_zoom, 1.0) ? m_pixmap : m_pixmap.scaledToWidth(qMax(int(m_pixmap.width() * m_zoom), 8), Qt::SmoothTransformation);
        break;
    }
    m_image->setPixmap(shown);
    m_image->adjustSize();
}

void FilePreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // A picture that fits the window or fills its width follows the size of the window.
    if (!m_pixmap.isNull() && m_imageMode != ImageMode::ActualSize) {
        showImage();
    }
}

bool FilePreviewWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_scroll->viewport() && event->type() == QEvent::Resize && !m_pixmap.isNull() && m_imageMode != ImageMode::ActualSize) {
        // The room for the picture changed: a picture that fits or fills it follows.
        QMetaObject::invokeMethod(this, &FilePreviewWidget::showImage, Qt::QueuedConnection);
    }
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
