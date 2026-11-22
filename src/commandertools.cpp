/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commandertools.h"

#include "kitemviews/kfileitemmodel.h"
#include "views/filepreviewwidget.h"

#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/PreviewJob>
#include <KLocalizedString>

#include <KIO/FileUndoManager>
#include <KIO/Global>
#include <KJob>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSpinBox>
#include <QTableWidget>
#include <QThread>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QtConcurrent>

QString CommanderTools::expandRenamePattern(const QString &pattern, const QString &fileName, int counter, const QDateTime &modified)
{
    const int dot = fileName.lastIndexOf(QLatin1Char('.'));
    const bool hasExtension = dot > 0 && dot < fileName.size() - 1;
    const QString base = hasExtension ? fileName.left(dot) : fileName;
    const QString extension = hasExtension ? fileName.mid(dot + 1) : QString();

    static const QRegularExpression token(QStringLiteral("\\[(N|E|F|d|t|C(\\d*))\\]"));
    QString result;
    qsizetype last = 0;
    for (auto it = token.globalMatch(pattern); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        result += pattern.mid(last, match.capturedStart() - last);
        last = match.capturedEnd();
        const QString name = match.captured(1);
        if (name == QLatin1String("N")) {
            result += base;
        } else if (name == QLatin1String("E")) {
            result += extension;
        } else if (name == QLatin1String("F")) {
            result += fileName;
        } else if (name == QLatin1String("d")) {
            result += modified.toString(QStringLiteral("yyyy-MM-dd"));
        } else if (name == QLatin1String("t")) {
            result += modified.toString(QStringLiteral("HHmmss"));
        } else {
            result += QStringLiteral("%1").arg(counter, match.captured(2).toInt(), 10, QLatin1Char('0'));
        }
    }
    result += pattern.mid(last);
    return result;
}

QString CommanderTools::hashFile(const QString &path, QCryptographicHash::Algorithm algorithm)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QCryptographicHash hash(algorithm);
    if (!hash.addData(&file)) {
        return QString();
    }
    return QString::fromLatin1(hash.result().toHex());
}

bool CommanderTools::checksumsMatch(const QString &a, const QString &b)
{
    const auto clean = [](const QString &s) {
        return s.simplified().remove(QLatin1Char(' ')).toLower();
    };
    return !clean(a).isEmpty() && clean(a) == clean(b);
}

QString CommanderTools::convertImage(const QString &source, const QString &format)
{
    const QImage image(source);
    if (image.isNull()) {
        return QString();
    }
    const QFileInfo info(source);
    const QString target = info.absolutePath() + QLatin1Char('/') + info.completeBaseName() + QLatin1Char('.') + format;
    if (QFile::exists(target) || !image.save(target, format.toLatin1().constData())) {
        return QString();
    }
    return target;
}

QString CommanderTools::toggledToDoName(const QString &fileName)
{
    if (fileName.endsWith(QLatin1String(".2do"), Qt::CaseInsensitive)) {
        return fileName.chopped(1) + QLatin1Char('x');
    }
    if (fileName.endsWith(QLatin1String(".2dx"), Qt::CaseInsensitive)) {
        return fileName.chopped(1) + QLatin1Char('o');
    }
    return fileName;
}

QString CommanderTools::saveClipboardAsFile(const QMimeData *clipboard, const QString &folder)
{
    if (!clipboard) {
        return QString();
    }
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH.mm.ss"));
    const auto freeName = [&](const QString &extension) {
        QString path = folder + QLatin1Char('/') + stamp + QLatin1Char('.') + extension;
        for (int i = 2; QFile::exists(path); ++i) {
            path = folder + QLatin1Char('/') + stamp + QLatin1Char(' ') + QString::number(i) + QLatin1Char('.') + extension;
        }
        return path;
    };
    if (clipboard->hasImage()) {
        const QImage image = qvariant_cast<QImage>(clipboard->imageData());
        const QString path = freeName(QStringLiteral("png"));
        return !image.isNull() && image.save(path, "PNG") ? path : QString();
    }
    if (clipboard->hasText()) {
        const QString path = freeName(QStringLiteral("txt"));
        QFile file(path);
        if (file.open(QIODevice::WriteOnly) && file.write(clipboard->text().toUtf8()) >= 0) {
            return path;
        }
    }
    return QString();
}

qint64 CommanderTools::folderSize(const QString &folder)
{
    qint64 total = 0;
    QDirIterator it(folder, QDir::Files | QDir::NoSymLinks | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

FolderSizesDialog::FolderSizesDialog(const QStringList &folders, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Folder Sizes"));
    resize(500, 400);
    auto *layout = new QVBoxLayout(this);
    auto *table = new QTableWidget(folders.size(), 2, this);
    table->setHorizontalHeaderLabels({i18nc("@title:column", "Folder"), i18nc("@title:column", "Size")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table);
    for (int row = 0; row < folders.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(QFileInfo(folders.at(row)).fileName()));
        table->setItem(row, 1, new QTableWidgetItem(i18nc("@info:status", "Calculating…")));
        auto *watcher = new QFutureWatcher<qint64>(this);
        const QString folder = folders.at(row);
        connect(watcher, &QFutureWatcherBase::finished, this, [=]() {
            // Rows move when the table is sorted, so find the row by its folder name.
            const auto found = table->findItems(QFileInfo(folder).fileName(), Qt::MatchExactly);
            if (!found.isEmpty()) {
                auto *cell = new QTableWidgetItem(KIO::convertSize(watcher->result()));
                cell->setData(Qt::UserRole, watcher->result());
                table->setItem(found.first()->row(), 1, cell);
            }
            watcher->deleteLater();
        });
        watcher->setFuture(QtConcurrent::run(&CommanderTools::folderSize, folder));
    }
}

QList<QStringList> CommanderTools::findDuplicates(const QString &folder)
{
    QHash<qint64, QStringList> bySize;
    QDirIterator it(folder, QDir::Files | QDir::NoSymLinks | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const qint64 size = it.fileInfo().size();
        if (size > 0) {
            bySize[size].append(path);
        }
    }
    QList<QStringList> groups;
    for (const QStringList &sameSize : std::as_const(bySize)) {
        if (sameSize.size() < 2) {
            continue;
        }
        QHash<QString, QStringList> byHash;
        for (const QString &path : sameSize) {
            const QString hash = hashFile(path, QCryptographicHash::Sha256);
            if (!hash.isEmpty()) {
                byHash[hash].append(path);
            }
        }
        for (QStringList group : std::as_const(byHash)) {
            if (group.size() > 1) {
                group.sort();
                groups.append(group);
            }
        }
    }
    std::sort(groups.begin(), groups.end(), [](const QStringList &a, const QStringList &b) {
        return a.first() < b.first();
    });
    return groups;
}

DuplicateFinderDialog::DuplicateFinderDialog(const QString &folder, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Duplicate Files in %1", folder));
    resize(700, 450);
    auto *layout = new QVBoxLayout(this);
    auto *status = new QLabel(i18nc("@info:status", "Searching…"), this);
    layout->addWidget(status);
    auto *tree = new QTreeWidget(this);
    tree->setHeaderHidden(true);
    layout->addWidget(tree);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto *trash = buttons->addButton(i18nc("@action:button", "Move Ticked to Trash"), QDialogButtonBox::ActionRole);
    trash->setEnabled(false);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    auto *watcher = new QFutureWatcher<QList<QStringList>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [=]() {
        const QList<QStringList> groups = watcher->result();
        status->setText(i18ncp("@info:status", "%1 group of identical files", "%1 groups of identical files", groups.size()));
        for (const QStringList &group : groups) {
            auto *parentItem =
                new QTreeWidgetItem(tree, {i18ncp("@item", "%1 copy of %2", "%1 copies of %2", group.size(), QFileInfo(group.first()).fileName())});
            for (int i = 0; i < group.size(); ++i) {
                auto *child = new QTreeWidgetItem(parentItem, {group.at(i)});
                child->setFlags(child->flags() | Qt::ItemIsUserCheckable);
                // The first copy is the one to keep: nothing is ticked for it.
                child->setCheckState(0, Qt::Unchecked);
            }
            parentItem->setExpanded(true);
        }
        trash->setEnabled(!groups.isEmpty());
    });
    watcher->setFuture(QtConcurrent::run(&CommanderTools::findDuplicates, folder));

    connect(trash, &QPushButton::clicked, this, [=, this]() {
        QList<QUrl> urls;
        for (QTreeWidgetItemIterator iter(tree); *iter; ++iter) {
            if ((*iter)->parent() && (*iter)->checkState(0) == Qt::Checked) {
                urls.append(QUrl::fromLocalFile((*iter)->text(0)));
            }
        }
        if (urls.isEmpty()) {
            return;
        }
        auto *job = KIO::trash(urls);
        connect(job, &KJob::result, this, [=]() {
            for (QTreeWidgetItemIterator iter(tree); *iter;) {
                QTreeWidgetItem *item = *iter;
                ++iter;
                if (item->parent() && item->checkState(0) == Qt::Checked) {
                    delete item;
                }
            }
        });
    });
}

FileViewDesignerDialog::FileViewDesignerDialog(const QList<QByteArray> &roles, const QList<int> &widths, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "File View Designer"));
    resize(520, 480);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(i18nc("@info", "Tick the columns to show and drag them into order with the buttons; the width is in pixels."), this));

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels(
        {i18nc("@title:column", "Show"), i18nc("@title:column", "Column"), i18nc("@title:column", "Width"), i18nc("@title:column", "Hide below")});
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    // The name comes first, then the shown columns in their order, then the others.
    QList<QByteArray> order = roles;
    order.removeAll("text");
    order.prepend("text");
    const QList<KFileItemModel::RoleInfo> infos = KFileItemModel::rolesInformation();
    for (const KFileItemModel::RoleInfo &info : infos) {
        if (!order.contains(info.role) && !info.requiresBaloo) {
            order << info.role;
        }
    }
    for (const QByteArray &role : std::as_const(order)) {
        QString title = QString::fromLatin1(role);
        for (const KFileItemModel::RoleInfo &info : infos) {
            if (info.role == role) {
                title = info.translation;
            }
        }
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *shown = new QTableWidgetItem();
        shown->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        shown->setCheckState(roles.contains(role) || role == "text" ? Qt::Checked : Qt::Unchecked);
        if (role == "text") {
            // The name is always the first column.
            shown->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        }
        m_table->setItem(row, 0, shown);
        auto *name = new QTableWidgetItem(title);
        name->setData(Qt::UserRole, role);
        name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 1, name);
        auto *width = new QSpinBox(m_table);
        width->setRange(30, 3000);
        const int index = roles.indexOf(role);
        width->setValue(index >= 0 && index < widths.size() ? widths.at(index) : 150);
        m_table->setCellWidget(row, 2, width);
        // The width of the view under which the column is hidden when columns are hidden for lack of room; 0 is the automatic rule.
        auto *hideBelow = new QSpinBox(m_table);
        hideBelow->setRange(0, 5000);
        hideBelow->setSpecialValueText(i18nc("@item", "Automatic"));
        hideBelow->setSuffix(i18nc("@item:valuesuffix pixels", " px"));
        m_table->setCellWidget(row, 3, hideBelow);
    }
    layout->addWidget(m_table);

    auto *moveRow = new QHBoxLayout();
    auto *up = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), i18nc("@action:button", "Move Up"), this);
    auto *down = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down")), i18nc("@action:button", "Move Down"), this);
    connect(up, &QPushButton::clicked, this, [this]() {
        moveCurrentRow(-1);
    });
    connect(down, &QPushButton::clicked, this, [this]() {
        moveCurrentRow(1);
    });
    moveRow->addWidget(up);
    moveRow->addWidget(down);
    moveRow->addStretch();
    layout->addLayout(moveRow);

    m_automatic = new QCheckBox(i18nc("@option:check", "Automatic widths"), this);
    m_automatic->setChecked(widths.isEmpty());
    layout->addWidget(m_automatic);
    connect(m_automatic, &QCheckBox::toggled, this, [this](bool automatic) {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            m_table->cellWidget(row, 2)->setEnabled(!automatic);
        }
    });
    for (int row = 0; row < m_table->rowCount(); ++row) {
        m_table->cellWidget(row, 2)->setEnabled(widths.isEmpty() ? false : true);
    }

    m_name = new QLineEdit(this);
    m_name->setPlaceholderText(i18nc("@info:placeholder", "Keep this design as a named file view (optional)"));
    layout->addWidget(m_name);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void FileViewDesignerDialog::moveCurrentRow(int by)
{
    const int row = m_table->currentRow();
    const int target = row + by;
    // The name, in the first row, stays where it is.
    if (row < 1 || target < 1 || target >= m_table->rowCount()) {
        return;
    }
    // The cells are swapped, one by one: the spin boxes keep their values.
    const auto swapCells = [this](int a, int b) {
        const Qt::CheckState shownA = m_table->item(a, 0)->checkState();
        const Qt::CheckState shownB = m_table->item(b, 0)->checkState();
        m_table->item(a, 0)->setCheckState(shownB);
        m_table->item(b, 0)->setCheckState(shownA);
        const QString nameA = m_table->item(a, 1)->text();
        const QVariant roleA = m_table->item(a, 1)->data(Qt::UserRole);
        m_table->item(a, 1)->setText(m_table->item(b, 1)->text());
        m_table->item(a, 1)->setData(Qt::UserRole, m_table->item(b, 1)->data(Qt::UserRole));
        m_table->item(b, 1)->setText(nameA);
        m_table->item(b, 1)->setData(Qt::UserRole, roleA);
        auto *widthA = static_cast<QSpinBox *>(m_table->cellWidget(a, 2));
        auto *widthB = static_cast<QSpinBox *>(m_table->cellWidget(b, 2));
        const int valueA = widthA->value();
        widthA->setValue(widthB->value());
        widthB->setValue(valueA);
        auto *hideA = static_cast<QSpinBox *>(m_table->cellWidget(a, 3));
        auto *hideB = static_cast<QSpinBox *>(m_table->cellWidget(b, 3));
        const int hiddenA = hideA->value();
        hideA->setValue(hideB->value());
        hideB->setValue(hiddenA);
    };
    swapCells(row, target);
    m_table->setCurrentCell(target, 1);
}

QList<QByteArray> FileViewDesignerDialog::roles() const
{
    QList<QByteArray> result;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->checkState() == Qt::Checked) {
            result << m_table->item(row, 1)->data(Qt::UserRole).toByteArray();
        }
    }
    return result;
}

QList<int> FileViewDesignerDialog::widths() const
{
    QList<int> result;
    if (m_automatic->isChecked()) {
        return result;
    }
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->checkState() == Qt::Checked) {
            result << static_cast<QSpinBox *>(m_table->cellWidget(row, 2))->value();
        }
    }
    return result;
}

QList<int> FileViewDesignerDialog::hideBelow() const
{
    QList<int> result;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->checkState() == Qt::Checked) {
            result << static_cast<QSpinBox *>(m_table->cellWidget(row, 3))->value();
        }
    }
    return result;
}

void FileViewDesignerDialog::setHideBelow(const QHash<QByteArray, int> &thresholds)
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const QByteArray role = m_table->item(row, 1)->data(Qt::UserRole).toByteArray();
        static_cast<QSpinBox *>(m_table->cellWidget(row, 3))->setValue(thresholds.value(role, 0));
    }
}

QString FileViewDesignerDialog::name() const
{
    return m_name->text().trimmed();
}

ChecksumDialog::ChecksumDialog(const KFileItem &item, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Checksum of %1", item.name()));
    auto *layout = new QVBoxLayout(this);

    static const struct {
        const char *label;
        QCryptographicHash::Algorithm algorithm;
    } kinds[3] = {{"MD5", QCryptographicHash::Md5}, {"SHA-1", QCryptographicHash::Sha1}, {"SHA-256", QCryptographicHash::Sha256}};

    const QString path = item.localPath();
    for (int i = 0; i < 3; ++i) {
        auto *row = new QLineEdit(this);
        row->setReadOnly(true);
        row->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        row->setMinimumWidth(row->fontMetrics().averageCharWidth() * 66);
        row->setText(path.isEmpty() ? i18nc("@info", "Only files on this computer can be checked.") : i18nc("@info:status", "Calculating…"));
        m_values[i] = row;
        layout->addWidget(new QLabel(QLatin1String(kinds[i].label), this));
        layout->addWidget(row);
        if (!path.isEmpty()) {
            // The hashing runs off the main thread, as a file can be large.
            QPointer<QLineEdit> target = row;
            const auto algorithm = kinds[i].algorithm;
            (void)QtConcurrent::run([path, algorithm, target]() {
                const QString hex = CommanderTools::hashFile(path, algorithm);
                if (target) {
                    QMetaObject::invokeMethod(target, [target, hex]() {
                        if (target) {
                            target->setText(hex.isEmpty() ? i18nc("@info", "The file cannot be read.") : hex);
                        }
                    });
                }
            });
        }
    }

    layout->addWidget(new QLabel(i18nc("@label", "Paste the checksum to compare with:"), this));
    m_expected = new QLineEdit(this);
    m_expected->setClearButtonEnabled(true);
    layout->addWidget(m_expected);
    m_verdict = new QLabel(this);
    layout->addWidget(m_verdict);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_expected, &QLineEdit::textChanged, this, &ChecksumDialog::updateVerdict);
    for (QLineEdit *row : m_values) {
        connect(row, &QLineEdit::textChanged, this, &ChecksumDialog::updateVerdict);
    }
}

void ChecksumDialog::updateVerdict()
{
    if (m_expected->text().trimmed().isEmpty()) {
        m_verdict->clear();
        return;
    }
    for (QLineEdit *row : m_values) {
        if (CommanderTools::checksumsMatch(row->text(), m_expected->text())) {
            m_verdict->setText(i18nc("@info", "The checksum matches."));
            return;
        }
    }
    m_verdict->setText(i18nc("@info", "The checksum does not match."));
}

BatchRenameDialog::BatchRenameDialog(const KFileItemList &items, QWidget *parent)
    : QDialog(parent)
    , m_items(items)
{
    setWindowTitle(i18nc("@title:window", "Rename %1 Items", items.count()));
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(
        new QLabel(i18nc("@info", "Pattern: [N] name, [E] extension, [F] whole name, [C] counter ([C3] with 3 digits), [d] date, [t] time"), this));
    m_pattern = new QLineEdit(QStringLiteral("[F]"), this);
    layout->addWidget(m_pattern);

    m_table = new QTableWidget(items.count(), 2, this);
    m_table->setHorizontalHeaderLabels({i18nc("@title:column", "Name"), i18nc("@title:column", "New name")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_table);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(i18nc("@action:button", "Rename"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &BatchRenameDialog::rename);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttons);

    connect(m_pattern, &QLineEdit::textChanged, this, &BatchRenameDialog::updatePreview);
    resize(520, 400);
    updatePreview();
}

QStringList BatchRenameDialog::newNames() const
{
    QStringList names;
    int counter = 1;
    for (const KFileItem &item : m_items) {
        names << CommanderTools::expandRenamePattern(m_pattern->text(), item.name(), counter++, item.time(KFileItem::ModificationTime));
    }
    return names;
}

void BatchRenameDialog::updatePreview()
{
    const QStringList names = newNames();
    QSet<QString> seen;
    QSet<QString> oldNames;
    for (const KFileItem &item : m_items) {
        oldNames.insert(item.name());
    }
    bool valid = true;
    for (int row = 0; row < names.size(); ++row) {
        const QString &name = names.at(row);
        // A name must not be empty or hold a slash, two files must not get the same name,
        // and a name must not be one of the other files' names that is still there.
        const bool clash =
            name.isEmpty() || name.contains(QLatin1Char('/')) || seen.contains(name) || (name != m_items.at(row).name() && oldNames.contains(name));
        seen.insert(name);
        valid = valid && !clash;
        m_table->setItem(row, 0, new QTableWidgetItem(m_items.at(row).name()));
        auto *newItem = new QTableWidgetItem(name);
        if (clash) {
            newItem->setForeground(Qt::red);
        }
        m_table->setItem(row, 1, newItem);
    }
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(valid && !m_items.isEmpty());
}

void BatchRenameDialog::rename()
{
    const QStringList names = newNames();
    for (int i = 0; i < names.size(); ++i) {
        if (names.at(i) == m_items.at(i).name()) {
            continue;
        }
        QUrl target = m_items.at(i).url().adjusted(QUrl::RemoveFilename);
        target.setPath(target.path() + names.at(i));
        KIO::moveAs(m_items.at(i).url(), target, KIO::HideProgressInfo);
    }
    accept();
}

QuickViewDialog::QuickViewDialog(QWidget *parent)
    : QDialog(parent)
{
    auto *layout = new QVBoxLayout(this);
    m_preview = new FilePreviewWidget(this);
    layout->addWidget(m_preview);
    resize(640, 480);
}

void QuickViewDialog::show(const KFileItem &item)
{
    setWindowTitle(i18nc("@title:window", "Quick View: %1", item.name()));
    m_preview->setItem(item);
    QDialog::show();
    raise();
}
