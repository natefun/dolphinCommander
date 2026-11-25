/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commandertools.h"

#include "kitemviews/kfileitemmodel.h"
#include "views/filepreviewwidget.h"

#include <KConfigGroup>
#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/PreviewJob>
#include <KLocalizedString>
#include <KSharedConfig>

#include <KFileMetaData/Extractor>
#include <KFileMetaData/ExtractorCollection>
#include <KFileMetaData/SimpleExtractionResult>
#include <KIO/FileUndoManager>
#include <KIO/Global>
#include <KJob>
#include <QAudioOutput>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDesktopServices>
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
#include <QIcon>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMediaPlayer>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QThread>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QVideoWidget>
#include <QtConcurrent>
#include <unistd.h>

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

QUrl CommanderTools::webLinkIn(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char('\n')) || trimmed.contains(QLatin1Char(' '))) {
        return QUrl();
    }
    const QUrl url(trimmed, QUrl::StrictMode);
    if (!url.isValid() || url.host().isEmpty()
        || !(url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https") || url.scheme() == QLatin1String("ftp"))) {
        return QUrl();
    }
    return url;
}

QString CommanderTools::saveLinkAsFile(const QUrl &url, const QString &folder)
{
    if (!url.isValid()) {
        return QString();
    }
    const QString name = url.host();
    QString path = folder + QLatin1Char('/') + name + QLatin1String(".desktop");
    for (int i = 2; QFile::exists(path); ++i) {
        path = folder + QLatin1Char('/') + name + QLatin1Char(' ') + QString::number(i) + QLatin1String(".desktop");
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return QString();
    }
    file.write("[Desktop Entry]\nType=Link\nName=" + name.toUtf8() + "\nURL=" + url.toString(QUrl::FullyEncoded).toUtf8() + "\nIcon=text-html\n");
    file.close();
    // A link file is shown by what it points to; it needs to be executable for some desktops to trust it.
    file.setPermissions(file.permissions() | QFileDevice::ExeOwner);
    return path;
}

QStringList CommanderTools::pasteAsLinks(const QList<QUrl> &urls, const QString &folder, bool hardLinks)
{
    QStringList made;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QFileInfo target(url.toLocalFile());
        if (!target.exists()) {
            continue;
        }
        QString path = folder + QLatin1Char('/') + target.fileName();
        for (int i = 2; QFileInfo::exists(path) || QFileInfo(path).isSymLink(); ++i) {
            path = folder + QLatin1Char('/') + target.completeBaseName() + QLatin1String(" - link")
                + (i > 2 ? QLatin1Char(' ') + QString::number(i) : QString()) + (target.suffix().isEmpty() ? QString() : QLatin1Char('.') + target.suffix());
        }
        if (hardLinks) {
            // A hard link is another name for the same file: not possible for a folder, or across file systems.
            if (!target.isDir() && ::link(QFile::encodeName(target.absoluteFilePath()).constData(), QFile::encodeName(path).constData()) == 0) {
                made << path;
            }
        } else if (QFile::link(target.absoluteFilePath(), path)) {
            made << path;
        }
    }
    return made;
}

std::optional<QPointF> CommanderTools::photoLocation(const QString &path)
{
    const QMimeType mime = QMimeDatabase().mimeTypeForFile(path);
    if (!mime.name().startsWith(QLatin1String("image/"))) {
        return std::nullopt;
    }
    KFileMetaData::ExtractorCollection extractors;
    for (KFileMetaData::Extractor *extractor : extractors.fetchExtractors(mime.name())) {
        KFileMetaData::SimpleExtractionResult result(path, mime.name(), KFileMetaData::ExtractionResult::ExtractMetaData);
        extractor->extract(&result);
        const auto properties = result.properties();
        const auto latitude = properties.constFind(KFileMetaData::Property::PhotoGpsLatitude);
        const auto longitude = properties.constFind(KFileMetaData::Property::PhotoGpsLongitude);
        if (latitude != properties.cend() && longitude != properties.cend()) {
            return QPointF(longitude.value().toDouble(), latitude.value().toDouble());
        }
    }
    return std::nullopt;
}

QUrl CommanderTools::mapUrl(const QPointF &location)
{
    const QString lat = QString::number(location.y(), 'f', 6);
    const QString lon = QString::number(location.x(), 'f', 6);
    return QUrl(QStringLiteral("https://www.openstreetmap.org/?mlat=%1&mlon=%2#map=16/%1/%2").arg(lat, lon));
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
    auto *temporary = buttons->addButton(i18nc("@action:button", "Apply Without Saving"), QDialogButtonBox::ActionRole);
    temporary->setObjectName(QStringLiteral("applyWithoutSaving"));
    connect(temporary, &QPushButton::clicked, this, [this]() {
        m_temporaryOnly = true;
        accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

bool FileViewDesignerDialog::temporaryOnly() const
{
    return m_temporaryOnly;
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

QString CommanderTools::templatesFolder()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TemplatesLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/Templates");
    }
    QDir().mkpath(dir);
    return dir;
}

QStringList CommanderTools::templateNames()
{
    return QDir(templatesFolder()).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
}

QString CommanderTools::createFromTemplate(const QString &templatePath, const QString &folder, const QString &name)
{
    if (name.isEmpty() || name.contains(QLatin1Char('/')) || !QFileInfo(templatePath).isFile()) {
        return QString();
    }
    const QString target = folder + QLatin1Char('/') + name;
    if (QFileInfo::exists(target) || !QFile::copy(templatePath, target)) {
        return QString();
    }
    return target;
}

QString CommanderTools::saveAsTemplate(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.isFile()) {
        return QString();
    }
    const QString folder = templatesFolder();
    QString target = folder + QLatin1Char('/') + info.fileName();
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix().isEmpty() ? QString() : QLatin1Char('.') + info.suffix();
    for (int i = 2; QFileInfo::exists(target); ++i) {
        target = folder + QLatin1Char('/') + base + QLatin1Char(' ') + QString::number(i) + suffix;
    }
    return QFile::copy(filePath, target) ? target : QString();
}

QString CommanderTools::formattedPath(const QString &path, PathFormat format)
{
    switch (format) {
    case PathFormat::Plain:
        return path;
    case PathFormat::Quoted:
        return QLatin1Char('"') + path + QLatin1Char('"');
    case PathFormat::ShellEscaped: {
        QString escaped;
        for (const QChar c : path) {
            if (!(c.isLetterOrNumber() || QStringLiteral("/._-+,:@%").contains(c))) {
                escaped += QLatin1Char('\\');
            }
            escaped += c;
        }
        return escaped;
    }
    case PathFormat::FileUrl:
        return QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
    }
    return path;
}

NewFromTemplateDialog::NewFromTemplateDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "New File from Template"));
    auto *layout = new QVBoxLayout(this);
    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("templateList"));
    for (const QString &name : CommanderTools::templateNames()) {
        auto *item = new QListWidgetItem(QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(name, QMimeDatabase::MatchExtension).iconName()), name, m_list);
        item->setData(Qt::UserRole, QString(CommanderTools::templatesFolder() + QLatin1Char('/') + name));
    }
    layout->addWidget(m_list);
    m_name = new QLineEdit(this);
    m_name->setObjectName(QStringLiteral("templateFileName"));
    m_name->setPlaceholderText(i18nc("@info:placeholder", "File name"));
    layout->addWidget(m_name);
    connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        if (item) {
            m_name->setText(item->text());
            m_name->selectAll();
        }
    });
    auto *buttons = new QDialogButtonBox(this);
    auto *create = buttons->addButton(i18nc("@action:button", "Create"), QDialogButtonBox::AcceptRole);
    auto *createOpen = buttons->addButton(i18nc("@action:button", "Create and Open"), QDialogButtonBox::AcceptRole);
    auto *edit = buttons->addButton(i18nc("@action:button", "Edit Templates"), QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    create->setDefault(true);
    layout->addWidget(buttons);
    connect(createOpen, &QPushButton::clicked, this, [this]() {
        m_open = true;
    });
    connect(edit, &QPushButton::clicked, this, [this]() {
        m_edit = true;
        accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
    m_name->setFocus();
}

QString NewFromTemplateDialog::selectedTemplate() const
{
    return m_list->currentItem() ? m_list->currentItem()->data(Qt::UserRole).toString() : QString();
}

QString NewFromTemplateDialog::fileName() const
{
    return m_name->text().trimmed();
}

bool NewFromTemplateDialog::openAfterwards() const
{
    return m_open;
}

bool NewFromTemplateDialog::editRequested() const
{
    return m_edit;
}

NewFolderDialog::NewFolderDialog(bool hasSelection, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "New Folder"));
    auto *layout = new QVBoxLayout(this);
    m_name = new QLineEdit(this);
    m_name->setObjectName(QStringLiteral("folderName"));
    m_name->setPlaceholderText(i18nc("@info:placeholder", "Folder name"));
    layout->addWidget(m_name);
    auto *buttons = new QDialogButtonBox(this);
    auto *create = buttons->addButton(i18nc("@action:button", "Create"), QDialogButtonBox::AcceptRole);
    auto *open = buttons->addButton(i18nc("@action:button", "Create and Open  (Ctrl+Enter)"), QDialogButtonBox::AcceptRole);
    m_moveButton = buttons->addButton(i18nc("@action:button", "Create and Move Selected Files Inside  (Shift+Enter)"), QDialogButtonBox::AcceptRole);
    m_moveButton->setEnabled(hasSelection);
    buttons->addButton(QDialogButtonBox::Cancel);
    create->setDefault(true);
    layout->addWidget(buttons);
    connect(create, &QPushButton::clicked, this, [this]() {
        m_result = Result::Create;
    });
    connect(open, &QPushButton::clicked, this, [this]() {
        m_result = Result::CreateAndOpen;
    });
    connect(m_moveButton, &QPushButton::clicked, this, [this]() {
        m_result = Result::CreateAndMoveSelected;
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_name->setFocus();
}

QString NewFolderDialog::folderName() const
{
    return m_name->text().trimmed();
}

void NewFolderDialog::setFolderName(const QString &name)
{
    m_name->setText(name);
}

NewFolderDialog::Result NewFolderDialog::result() const
{
    return m_result;
}

void NewFolderDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() & Qt::ControlModifier) {
            m_result = Result::CreateAndOpen;
            accept();
            return;
        }
        if ((event->modifiers() & Qt::ShiftModifier) && m_moveButton->isEnabled()) {
            m_result = Result::CreateAndMoveSelected;
            accept();
            return;
        }
    }
    QDialog::keyPressEvent(event);
}

QuickViewDialog::QuickViewDialog(QWidget *parent)
    : QDialog(parent)
{
    auto *layout = new QVBoxLayout(this);
    m_preview = new FilePreviewWidget(this);
    layout->addWidget(m_preview);
    // How a picture is laid out: all of it in the window, as wide as the window, or at its own size.
    auto *modes = new QWidget(this);
    modes->setObjectName(QStringLiteral("imageModes"));
    auto *modesLayout = new QHBoxLayout(modes);
    modesLayout->setContentsMargins(0, 0, 0, 0);
    auto *group = new QButtonGroup(modes);
    const struct {
        const char *label;
        FilePreviewWidget::ImageMode mode;
    } imageModes[] = {{"Fit", FilePreviewWidget::ImageMode::Fit},
                      {"Fill Width", FilePreviewWidget::ImageMode::FillWidth},
                      {"Actual Size", FilePreviewWidget::ImageMode::ActualSize}};
    for (const auto &entry : imageModes) {
        auto *button = new QPushButton(i18nc("@action:button how a picture is shown", entry.label), modes);
        button->setCheckable(true);
        button->setChecked(entry.mode == FilePreviewWidget::ImageMode::Fit);
        group->addButton(button);
        const auto mode = entry.mode;
        connect(button, &QPushButton::clicked, this, [this, mode]() {
            m_preview->setImageMode(mode);
        });
        modesLayout->addWidget(button);
    }
    // OneCommander's "Image scaling interpolation": smooth, or pixelated for sharp pixels.
    auto *pixelated = new QCheckBox(i18nc("@option:check", "Pixelated scaling"), modes);
    pixelated->setObjectName(QStringLiteral("pixelatedScaling"));
    pixelated->setChecked(!KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("QuickViewSmooth", true));
    m_preview->setSmoothScaling(!pixelated->isChecked());
    connect(pixelated, &QCheckBox::toggled, this, [this](bool on) {
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("QuickViewSmooth", !on);
        m_preview->setSmoothScaling(!on);
    });
    modesLayout->addWidget(pixelated);
    modesLayout->addStretch();
    modes->hide();
    layout->addWidget(modes);
    m_mapButton = new QPushButton(QIcon::fromTheme(QStringLiteral("globe")), i18nc("@action:button", "Show on Map"), this);
    m_mapButton->hide();
    connect(m_mapButton, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(m_mapLink);
    });
    layout->addWidget(m_mapButton);
    auto *autoplay = new QCheckBox(i18nc("@option:check", "Play videos and sounds at once"), this);
    autoplay->setObjectName(QStringLiteral("autoplay"));
    autoplay->setChecked(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("QuickViewAutoplay", false));
    connect(autoplay, &QCheckBox::toggled, this, [](bool on) {
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("QuickViewAutoplay", on);
    });
    layout->addWidget(autoplay);
    m_video = new QVideoWidget(this);
    m_video->hide();
    layout->addWidget(m_video, 1);
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_video);
    auto *audio = new QAudioOutput(this);
    m_player->setAudioOutput(audio);
    resize(640, 480);
}

void QuickViewDialog::seekBy(qint64 milliseconds)
{
    if (m_player->duration() <= 0) {
        return;
    }
    m_player->setPosition(qBound<qint64>(0, m_player->position() + milliseconds, m_player->duration()));
}

void QuickViewDialog::stepFrame(int direction)
{
    if (m_player->duration() <= 0) {
        return;
    }
    m_player->pause();
    m_player->setPosition(qBound<qint64>(0, m_player->position() + direction * 40, m_player->duration()));
}

void QuickViewDialog::togglePlayback()
{
    if (m_player->duration() <= 0) {
        return;
    }
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}

qint64 QuickViewDialog::mediaPosition() const
{
    return m_player->position();
}

qint64 QuickViewDialog::mediaDuration() const
{
    return qMax<qint64>(0, m_player->duration());
}

QUrl QuickViewDialog::mapLink() const
{
    return m_mapLink;
}

QString QuickViewDialog::shownName() const
{
    return m_shownName;
}

void QuickViewDialog::keyPressEvent(QKeyEvent *event)
{
    // Video and sound: Space plays and pauses, Left and Right seek (Shift: an eighth of the length, as OneCommander), [ and ] step by frames.
    if (m_player->duration() > 0) {
        const qint64 jump = (event->modifiers() & Qt::ShiftModifier) ? qMax<qint64>(1000, m_player->duration() / 8) : 5000;
        switch (event->key()) {
        case Qt::Key_Space:
            togglePlayback();
            return;
        case Qt::Key_Left:
            seekBy(-jump);
            return;
        case Qt::Key_Right:
            seekBy(jump);
            return;
        case Qt::Key_BracketLeft:
            stepFrame(-1);
            return;
        case Qt::Key_BracketRight:
            stepFrame(1);
            return;
        default:
            break;
        }
    }
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        Q_EMIT stepRequested(event->key() == Qt::Key_Up ? -1 : 1);
        return;
    }
    QDialog::keyPressEvent(event);
}

void QuickViewDialog::show(const KFileItem &item)
{
    setWindowTitle(i18nc("@title:window", "Quick View: %1", item.name()));
    m_shownName = item.name();
    m_player->stop();
    const QMimeType mime = item.determineMimeType();
    const bool isMedia = item.isLocalFile() && (mime.name().startsWith(QLatin1String("video/")) || mime.name().startsWith(QLatin1String("audio/")));
    if (isMedia) {
        // A video or a sound is played, not shown as a picture.
        m_preview->setItem(KFileItem());
        m_video->setVisible(mime.name().startsWith(QLatin1String("video/")));
        m_player->setSource(QUrl::fromLocalFile(item.localPath()));
    } else {
        m_video->hide();
        m_player->setSource(QUrl());
        m_preview->setItem(item);
    }
    // A photo with a position has a button for the map.
    m_mapLink = QUrl();
    if (item.isLocalFile()) {
        if (const auto location = CommanderTools::photoLocation(item.localPath())) {
            m_mapLink = CommanderTools::mapUrl(*location);
        }
    }
    m_mapButton->setVisible(m_mapLink.isValid());
    QWidget *imageModes = findChild<QWidget *>(QStringLiteral("imageModes"));
    imageModes->setVisible(m_preview->showsImage());
    if (m_preview->showsImage()) {
        // The mode that suits the picture is chosen, and its button shows it; the others can still be pressed.
        const int guessed = int(m_preview->guessImageMode());
        const QList<QPushButton *> buttons = imageModes->findChildren<QPushButton *>();
        for (int i = 0; i < buttons.size(); ++i) {
            buttons.at(i)->setChecked(i == guessed);
        }
    }
    if (isMedia && KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("QuickViewAutoplay", false)) {
        m_player->play();
    }
    QDialog::show();
    raise();
}
