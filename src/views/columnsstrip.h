/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COLUMNSSTRIP_H
#define COLUMNSSTRIP_H

#include "dolphin_export.h"

#include <QList>
#include <QSplitter>
#include <QUrl>

class KFileItem;
class KFileItemModel;
class KItemListController;
class QResizeEvent;

/**
 * @brief Miller columns: a horizontal strip with one column per folder of the current path.
 *
 * The columns share the width of the strip and can be resized by dragging the dividers
 * between them. If there is not enough room for all of them, the columns that are farthest
 * back in the path are hidden, so the strip never needs to be scrolled.
 *
 * Every column lists the contents of one folder and highlights the child that leads to the
 * next column. Clicking a folder asks for navigation to it via urlRequested() and the owner
 * answers by calling setUrl(), which reuses matching columns and trims or appends the rest.
 */
class DOLPHIN_EXPORT ColumnsStrip : public QSplitter
{
    Q_OBJECT

public:
    explicit ColumnsStrip(QWidget *parent = nullptr);
    ~ColumnsStrip() override;

    /** Shows the columns for @p url. Does not emit urlRequested(). */
    void setUrl(const QUrl &url);
    QUrl url() const;

    void setShowHiddenFiles(bool show);

Q_SIGNALS:
    /** Emitted when the user picks a folder in any column. */
    void urlRequested(const QUrl &url);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    struct Column {
        QUrl url;
        QWidget *widget = nullptr;
        KFileItemModel *model = nullptr;
        KItemListController *controller = nullptr;
    };

    Column createColumn(const QUrl &url);
    void selectChild(Column &column, const QUrl &childUrl);
    void slotItemActivated(const Column &column, int index);
    /** Hides the columns that do not fit and distributes the width among the others. */
    void updateColumnLayout();

private:
    QList<Column> m_columns;
    /** Takes the width the columns do not need, so that they do not have to be stretched. */
    QWidget *m_filler = nullptr;
    /** The width wanted for each column, by position in the path. It is what the user dragged them to. */
    QList<int> m_columnWidths;
    int m_firstVisibleColumn = 0;
    QUrl m_url;
    bool m_showHiddenFiles = false;
};

#endif
