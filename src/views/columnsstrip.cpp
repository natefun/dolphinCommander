/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "columnsstrip.h"

#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"

#include <KFileItem>
#include <KIO/OpenUrlJob>
#include <KJobUiDelegate>
#include <KNotificationJobUiDelegate>

#include <QPointer>
#include <QResizeEvent>
#include <QTimer>

namespace
{
/** A column is never narrower than this, even if the user drags a divider. */
constexpr int MinimumColumnWidth = 80;
/** Columns are hidden, farthest back first, until the rest can have at least this much. */
constexpr int PreferredColumnWidth = 130;
/** The width a new column starts with. Columns are only made narrower than this if there is not enough room. */
constexpr int DefaultColumnWidth = 170;
constexpr int StripHeight = 190;

QUrl parentUrl(const QUrl &url)
{
    QUrl up = url.adjusted(QUrl::StripTrailingSlash);
    if (up.path().isEmpty() || up.path() == QLatin1String("/")) {
        return QUrl();
    }
    return up.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
}
}

ColumnsStrip::ColumnsStrip(QWidget *parent)
    : QSplitter(Qt::Horizontal, parent)
{
    setChildrenCollapsible(false);
    setHandleWidth(3);
    setFixedHeight(StripHeight);

    m_filler = new QWidget(this);
    addWidget(m_filler);

    // Remember what the user dragged, but only for the columns that are shown.
    connect(this, &QSplitter::splitterMoved, this, [this]() {
        const QList<int> widths = sizes();
        for (int i = m_firstVisibleColumn; i < m_columns.size() && i < widths.size(); ++i) {
            m_columnWidths[i] = qMax(widths.at(i), 1);
        }
    });
}

ColumnsStrip::~ColumnsStrip() = default;

QUrl ColumnsStrip::url() const
{
    return m_url;
}

void ColumnsStrip::setShowHiddenFiles(bool show)
{
    m_showHiddenFiles = show;
    for (Column &column : m_columns) {
        column.model->setShowHiddenFiles(show);
    }
}

void ColumnsStrip::setUrl(const QUrl &url)
{
    if (url == m_url && !m_columns.isEmpty()) {
        return;
    }
    m_url = url;

    // Path from the root down to the shown folder.
    QList<QUrl> path;
    for (QUrl current = url.adjusted(QUrl::StripTrailingSlash); current.isValid(); current = parentUrl(current)) {
        path.prepend(current);
        if (parentUrl(current).isEmpty()) {
            break;
        }
    }

    // The folder being shown is already listed by the view below the strip, so the
    // strip only needs the folders leading to it (unless there are none).
    QList<QUrl> columnUrls = path;
    if (columnUrls.size() > 1) {
        columnUrls.removeLast();
    }

    // Keep the longest common prefix of existing columns, drop the rest.
    int keep = 0;
    while (keep < m_columns.size() && keep < columnUrls.size() && m_columns.at(keep).url.adjusted(QUrl::StripTrailingSlash) == columnUrls.at(keep)) {
        ++keep;
    }
    while (m_columns.size() > keep) {
        Column column = m_columns.takeLast();
        column.widget->hide();
        column.widget->setParent(nullptr);
        column.widget->deleteLater();
    }
    for (int i = keep; i < columnUrls.size(); ++i) {
        Column column = createColumn(columnUrls.at(i));
        insertWidget(m_columns.size(), column.widget);
        m_columns.append(column);
    }
    while (m_columnWidths.size() < m_columns.size()) {
        m_columnWidths.append(DefaultColumnWidth);
    }
    while (m_columnWidths.size() > m_columns.size()) {
        m_columnWidths.removeLast();
    }
    // Every column highlights the folder that leads to the next one, the last column
    // highlights the folder shown in the view.
    for (int i = 0; i < m_columns.size() && i + 1 < path.size(); ++i) {
        selectChild(m_columns[i], path.at(i + 1));
    }

    updateColumnLayout();
}

void ColumnsStrip::resizeEvent(QResizeEvent *event)
{
    QSplitter::resizeEvent(event);
    updateColumnLayout();
}

void ColumnsStrip::showEvent(QShowEvent *event)
{
    QSplitter::showEvent(event);
    updateColumnLayout();
}

void ColumnsStrip::updateColumnLayout()
{
    const int count = m_columns.size();
    if (count == 0 || width() <= 0) {
        return;
    }

    // Show as many of the last columns as have room, but at least one.
    const int visibleCount = qBound(1, width() / PreferredColumnWidth, count);
    m_firstVisibleColumn = count - visibleCount;

    // Every column gets the width that was asked for, or less if they do not all fit. What is
    // left over goes to the filler behind the last column.
    const int available = width() - visibleCount * handleWidth();
    int wanted = 0;
    for (int i = m_firstVisibleColumn; i < count; ++i) {
        wanted += m_columnWidths.at(i);
    }
    const double scale = wanted > available ? double(available) / wanted : 1.0;
    QList<int> newSizes;
    int used = 0;
    for (int i = 0; i < count; ++i) {
        const bool visible = i >= m_firstVisibleColumn;
        m_columns.at(i).widget->setVisible(visible);
        const int columnWidth = visible ? qMax(MinimumColumnWidth, int(m_columnWidths.at(i) * scale)) : 0;
        newSizes.append(columnWidth);
        used += columnWidth;
    }
    newSizes.append(qMax(0, available - used));
    setSizes(newSizes);
}

ColumnsStrip::Column ColumnsStrip::createColumn(const QUrl &url)
{
    Column column;
    column.url = url;

    auto *view = new KFileItemListView();
    view->setItemLayout(KFileItemListView::DetailsLayout);
    view->setVisibleRoles({"text"});
    view->setEnabledSelectionToggles(false);

    column.model = new KFileItemModel(this);
    column.model->setShowHiddenFiles(m_showHiddenFiles);

    column.controller = new KItemListController(column.model, view, this);
    column.controller->setSelectionBehavior(KItemListController::SingleSelection);
    column.controller->setSingleClickActivationEnforced(true);

    auto *container = new KItemListContainer(column.controller);
#ifndef QT_NO_ACCESSIBILITY
    view->setAccessibleParentsObject(container);
#endif
    container->setEnabledFrame(false);
    container->setMinimumWidth(MinimumColumnWidth);
    // Long names are cut off instead of making the column scroll sideways.
    container->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    column.widget = container;

    // The model and controller are parented to this widget so they outlive the QObject tree
    // of the container, but they must not stay around after their column is deleted.
    connect(container, &QObject::destroyed, column.model, &QObject::deleteLater);
    connect(container, &QObject::destroyed, column.controller, &QObject::deleteLater);

    // Column storage may be reallocated, so look the column up by url when it is used.
    const QUrl columnUrl = url;
    connect(column.controller, &KItemListController::itemActivated, this, [this, columnUrl](int index) {
        for (const Column &c : std::as_const(m_columns)) {
            if (c.url == columnUrl) {
                slotItemActivated(c, index);
                return;
            }
        }
    });

    column.model->loadDirectory(url);
    return column;
}

void ColumnsStrip::selectChild(Column &column, const QUrl &childUrl)
{
    auto select = [model = QPointer<KFileItemModel>(column.model), controller = QPointer<KItemListController>(column.controller), childUrl]() {
        if (!model || !controller) {
            return;
        }
        const int index = model->index(childUrl);
        if (index >= 0) {
            controller->selectionManager()->setCurrentItem(index);
            controller->selectionManager()->clearSelection();
            controller->selectionManager()->setSelected(index);
        }
    };
    select();
    // The listing may not be finished yet.
    connect(column.model, &KFileItemModel::directoryLoadingCompleted, column.model, select, Qt::SingleShotConnection);
}

void ColumnsStrip::slotItemActivated(const Column &column, int index)
{
    const KFileItem item = column.model->fileItem(index);
    if (item.isNull()) {
        return;
    }
    if (item.isDir()) {
        Q_EMIT urlRequested(item.targetUrl());
        return;
    }
    auto *job = new KIO::OpenUrlJob(item.targetUrl(), item.mimetype(), this);
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->start();
}
