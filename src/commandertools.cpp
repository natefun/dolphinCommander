/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commandertools.h"

#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/PreviewJob>
#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTableWidget>
#include <QThread>
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
    setAttribute(Qt::WA_DeleteOnClose, false);
    auto *layout = new QVBoxLayout(this);
    m_title = new QLabel(this);
    layout->addWidget(m_title);
    m_image = new QLabel(this);
    m_image->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_image);
    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_text);
    resize(640, 480);
}

void QuickViewDialog::show(const KFileItem &item)
{
    setWindowTitle(i18nc("@title:window", "Quick View: %1", item.name()));
    m_title->setText(item.name());
    m_image->clear();
    m_text->clear();
    m_image->hide();
    m_text->hide();

    const QString path = item.localPath();
    const QMimeType mime = item.determineMimeType();
    if (!path.isEmpty() && mime.inherits(QStringLiteral("text/plain"))) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            m_text->setPlainText(QString::fromUtf8(file.read(64 * 1024)));
            m_text->show();
        }
    } else if (!path.isEmpty() || mime.name().startsWith(QLatin1String("image/"))) {
        auto *job = new KIO::PreviewJob(KFileItemList{item}, QSize(600, 400));
        job->setIgnoreMaximumSize(false);
        QPointer<QuickViewDialog> self(this);
        connect(job, &KIO::PreviewJob::gotPreview, this, [self](const KFileItem &, const QPixmap &pixmap) {
            if (self) {
                self->m_image->setPixmap(pixmap);
                self->m_image->show();
            }
        });
        connect(job, &KIO::PreviewJob::failed, this, [self, item]() {
            if (self) {
                self->m_title->setText(i18nc("@info", "%1 — no preview for this kind of file.", item.name()));
            }
        });
        job->start();
    } else {
        m_title->setText(i18nc("@info", "%1 — no preview for this kind of file.", item.name()));
    }
    QDialog::show();
    raise();
}
