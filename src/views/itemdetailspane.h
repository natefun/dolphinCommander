/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ITEMDETAILSPANE_H
#define ITEMDETAILSPANE_H

#include "dolphin_export.h"

#include <KFileItem>

#include <QWidget>

class QLabel;

/**
 * The details pane under the file list of one side, as OneCommander has it: a picture of the selected file
 * and its name, kind, dates, age and size, with the number of items and of the selected ones above.
 * With nothing selected it describes the folder.
 */
class DOLPHIN_EXPORT ItemDetailsPane : public QWidget
{
    Q_OBJECT
public:
    explicit ItemDetailsPane(QWidget *parent = nullptr);

    /** Describes @p selection out of @p itemCount items of the folder @p folder. */
    void setContents(const KFileItem &folder, int itemCount, const KFileItemList &selection);

    /** "7 d", "9 M" or "2 y": how long ago @p time was. */
    static QString ageText(const QDateTime &time, const QDateTime &now = QDateTime::currentDateTime());

    /** The text of the line with the counts, such as "11 items, 1 selected, 2.3 MiB". */
    QString summaryText() const;
    /** The text of one row ("Name", "Type", "Created", "Modified", "Age", "Size"), empty if there is none. */
    QString valueOf(const QString &key) const;

    QSize sizeHint() const override;

private:
    QLabel *m_picture;
    QLabel *m_summary;
    QList<QPair<QLabel *, QLabel *>> m_rows;
    quint64 m_generation = 0;
};

#endif
