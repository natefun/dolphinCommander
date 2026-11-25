/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "columnsstrip.h"

#include "commandersettings.h"

#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"

#include <KFileItem>
#include <KIO/OpenUrlJob>
#include <KJobUiDelegate>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>

#include <QEnterEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QSplitterHandle>
#include <QTimer>
#include <QVBoxLayout>

#include <functional>

namespace
{
/** A column is never narrower than this, even if the user drags a divider. */
int minimumColumnWidth()
{
    // OneCommander's "Columns minimum width".
    return qBound(40, CommanderSettings::intValue(QStringLiteral("ColumnMinimumWidth"), 80), 400);
}
/** Columns are hidden, farthest back first, until the rest can have at least this much. */
int preferredColumnWidth()
{
    // OneCommander's "Columns small width": the width the left-most columns contract to when there is no room.
    return qBound(40, CommanderSettings::intValue(QStringLiteral("ColumnWidthSmall"), 150), 400);
}
/** The width a new column starts with. Columns are only made narrower than this if there is not enough room. */
int defaultColumnWidth()
{
    // OneCommander's "Current (rightmost) column pane width in Columns layout".
    return qBound(80, CommanderSettings::intValue(QStringLiteral("ColumnWidthCurrentColumnsLayout"), 170), 1000);
}
constexpr int StripHeight = 224;
/** The width of the strip that stands for a column that is hidden. */
constexpr int CollapsedLabelWidth = 14;

/** A narrow strip with the name of a folder written upwards. A click on it opens the folder. */
class CollapsedLabel : public QWidget
{
public:
    CollapsedLabel(const QString &text, const std::function<void()> &clicked, QWidget *parent)
        : QWidget(parent)
        , m_text(text)
        , m_clicked(clicked)
    {
        setFixedWidth(CollapsedLabelWidth);
        QFont smaller = font();
        smaller.setPointSizeF(smaller.pointSizeF() * 0.85);
        setFont(smaller);
        setCursor(Qt::PointingHandCursor);
        setToolTip(text);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), palette().alternateBase());
        painter.setPen(palette().color(QPalette::Disabled, QPalette::WindowText));
        painter.translate(0, height());
        painter.rotate(-90);
        painter.drawText(QRect(4, 0, height() - 8, width()), Qt::AlignVCenter | Qt::AlignLeft, fontMetrics().elidedText(m_text, Qt::ElideRight, height() - 8));
    }
    void mouseReleaseEvent(QMouseEvent *) override
    {
        m_clicked();
    }

private:
    QString m_text;
    std::function<void()> m_clicked;
};

QUrl parentUrl(const QUrl &url)
{
    QUrl up = url.adjusted(QUrl::StripTrailingSlash);
    if (up.path().isEmpty() || up.path() == QLatin1String("/")) {
        return QUrl();
    }
    return up.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
}
}

namespace
{
bool s_resizingLocked = false;

class LockableHandle : public QSplitterHandle
{
public:
    using QSplitterHandle::QSplitterHandle;

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (!s_resizingLocked) {
            QSplitterHandle::mousePressEvent(event);
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!s_resizingLocked) {
            QSplitterHandle::mouseMoveEvent(event);
        }
    }
    void enterEvent(QEnterEvent *event) override
    {
        setCursor(s_resizingLocked ? Qt::ArrowCursor : Qt::SplitHCursor);
        QSplitterHandle::enterEvent(event);
    }
};
}

void ColumnsStrip::setResizingLocked(bool locked)
{
    s_resizingLocked = locked;
}

bool ColumnsStrip::isResizingLocked()
{
    return s_resizingLocked;
}

QSplitterHandle *ColumnsStrip::createHandle()
{
    return new LockableHandle(orientation(), this);
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
        const int offset = m_columns.size();
        for (int i = m_firstVisibleColumn; i < m_columns.size() && offset + i < widths.size(); ++i) {
            m_columnWidths[i] = qMax(widths.at(offset + i), 1);
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
        column.label->hide();
        column.label->setParent(nullptr);
        column.label->deleteLater();
    }
    for (int i = keep; i < columnUrls.size(); ++i) {
        Column column = createColumn(columnUrls.at(i));
        // The labels of all columns come first, then the columns themselves.
        insertWidget(m_columns.size(), column.label);
        insertWidget(m_columns.size() * 2 + 1, column.widget);
        m_columns.append(column);
    }
    while (m_columnWidths.size() < m_columns.size()) {
        m_columnWidths.append(defaultColumnWidth());
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

    // Show as many of the last columns as have room, but at least one. The columns that do not fit are
    // narrow labels at the left, which takes some of the room.
    int visibleCount = 1;
    for (int v = count; v > 1; --v) {
        if ((width() - (count - v) * CollapsedLabelWidth) / v >= preferredColumnWidth()) {
            visibleCount = v;
            break;
        }
    }
    m_firstVisibleColumn = count - visibleCount;

    // Every column gets the width that was asked for, or less if they do not all fit. What is
    // left over goes to the filler behind the last column.
    const int available = width() - visibleCount * handleWidth() - m_firstVisibleColumn * (CollapsedLabelWidth + handleWidth());
    int wanted = 0;
    for (int i = m_firstVisibleColumn; i < count; ++i) {
        wanted += m_columnWidths.at(i);
    }
    const double scale = wanted > available ? double(available) / wanted : 1.0;
    QList<int> newSizes;
    for (int i = 0; i < count; ++i) {
        const bool collapsed = i < m_firstVisibleColumn;
        m_columns.at(i).label->setVisible(collapsed);
        newSizes.append(collapsed ? CollapsedLabelWidth : 0);
    }
    int used = 0;
    for (int i = 0; i < count; ++i) {
        const bool visible = i >= m_firstVisibleColumn;
        m_columns.at(i).widget->setVisible(visible);
        const int columnWidth = visible ? qMax(minimumColumnWidth(), int(m_columnWidths.at(i) * scale)) : 0;
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
    container->setMinimumWidth(minimumColumnWidth());
    // Long names are cut off instead of making the column scroll sideways.
    container->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // The name of the folder is above its list.
    auto *wrapper = new QWidget(this);
    auto *wrapperLayout = new QVBoxLayout(wrapper);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->setSpacing(0);
    const QString folderName = url.adjusted(QUrl::StripTrailingSlash).fileName();
    auto *title = new QLabel(folderName.isEmpty() ? i18nc("@title the root folder", "Root") : folderName, wrapper);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.4);
    title->setFont(titleFont);
    title->setContentsMargins(4, 2, 4, 2);
    title->setMinimumWidth(1);
    title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    title->setToolTip(url.toDisplayString(QUrl::PreferLocalFile));
    wrapperLayout->addWidget(title);
    wrapperLayout->addWidget(container, 1);
    wrapper->setMinimumWidth(minimumColumnWidth());
    column.widget = wrapper;

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

    const QString name = url.adjusted(QUrl::StripTrailingSlash).fileName();
    column.label = new CollapsedLabel(
        name.isEmpty() ? i18nc("@title the root folder", "Root") : name,
        [this, url]() {
            Q_EMIT urlRequested(url);
        },
        this);
    column.label->hide();

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
