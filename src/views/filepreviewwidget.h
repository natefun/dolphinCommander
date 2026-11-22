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
class QScrollArea;

/** Shows what is in a file: a preview of an image or a document, or the start of a text. Used by Quick View and the preview of the columns view. */
class DOLPHIN_EXPORT FilePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FilePreviewWidget(QWidget *parent = nullptr);

    /** Shows the file. A folder or a null item shows nothing. */
    void setItem(const KFileItem &item);

    /** The name of the item that is shown, empty if there is none. */
    QString shownName() const;

    QSize sizeHint() const override;

    /** The zoom of the shown image: 1 fits it to the width, the mouse wheel zooms it and a drag moves it. */
    qreal zoom() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLabel *m_title;
    void showImage();

    QScrollArea *m_scroll;
    QLabel *m_image;
    QPixmap m_pixmap;
    qreal m_zoom = 1;
    QPoint m_dragFrom;
    QPlainTextEdit *m_text;
    QString m_shownName;
    quint64 m_generation = 0;
};

#endif
