/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILEPREVIEWWIDGET_H
#define FILEPREVIEWWIDGET_H

#include "dolphin_export.h"

#include <KFileItem>

#include <QPixmap>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QResizeEvent;
class QScrollArea;

/** Shows what is in a file: a preview of an image or a document, or the start of a text. Used by Quick View and the preview of the columns view. */
class DOLPHIN_EXPORT FilePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FilePreviewWidget(QWidget *parent = nullptr);

    /** Shows the file. A folder or a null item shows nothing. */
    void setItem(const KFileItem &item);

    /**
     * The text of an office document: a .docx (Word), .xlsx (Excel), or an OpenDocument .odt, .ods or .odp file.
     * Empty if the file is none of these or cannot be read. At most about 64 kilobytes are returned.
     */
    static QString officeDocumentText(const QString &path);

    /** The name of the item that is shown, empty if there is none. */
    QString shownName() const;

    QSize sizeHint() const override;

    /** How an image is laid out: fitted whole into the window, as wide as the window (and scrolled down), or at its own size. */
    enum class ImageMode {
        Fit,
        FillWidth,
        ActualSize
    };
    /** The mode that suits a picture of @p image pixels in a window of @p room pixels: its own size if it fits, as wide as the window if it is very wide or
     * very tall, else all of it. */
    static ImageMode bestModeFor(const QSize &image, const QSize &room);
    /** Picks the mode that suits the picture that is shown (OneCommander guesses it too) and returns it. */
    ImageMode guessImageMode();
    void setImageMode(ImageMode mode);
    ImageMode imageMode() const;
    /** True while a picture is shown. */
    bool showsImage() const;
    /** The size of the picture as it is drawn now, empty when there is none. */
    QSize shownImageSize() const;

    /** The zoom of the shown image: 1 fits it to the width, the mouse wheel zooms it and a drag moves it. */
    qreal zoom() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *m_title;
    void showImage();

    QScrollArea *m_scroll;
    QLabel *m_image;
    QPixmap m_pixmap;
    qreal m_zoom = 1;
    ImageMode m_imageMode = ImageMode::Fit;
    QPoint m_dragFrom;
    QPlainTextEdit *m_text;
    QString m_shownName;
    quint64 m_generation = 0;
};

#endif
