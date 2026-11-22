/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COMMANDERTOOLS_H
#define COMMANDERTOOLS_H

#include <KFileItem>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QList>
#include <QStringList>
#include <QUrl>

class FilePreviewWidget;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QDialogButtonBox;

/** What dolphinCommander adds beyond Dolphin: helpers and dialogs for checksums, batch renaming and quick view. */
namespace CommanderTools
{
/**
 * Expands a batch rename pattern for one file:
 * [N] the name without its last extension, [E] the extension, [F] the whole name,
 * [C] a counter that starts at 1, [C3] the same padded with zeros to 3 digits (any width),
 * [d] the modification date as 2026-11-21, [t] the modification time as 154500.
 * Everything else, including a lone "[", stays as it is.
 */
QString expandRenamePattern(const QString &pattern, const QString &fileName, int counter, const QDateTime &modified);

/** The hex digest of the file's content, or an empty string if it cannot be read. */
QString hashFile(const QString &path, QCryptographicHash::Algorithm algorithm);

/** True when two hex digests are the same, ignoring case and the whitespace a pasted one brings. */
bool checksumsMatch(const QString &a, const QString &b);

/** Groups of files below @p folder (recursively, without symlinks) that have the same content. Empty files are ignored. */
QList<QStringList> findDuplicates(const QString &folder);
}

/** Lists the duplicates below a folder and moves the ticked ones to the trash. */
class DuplicateFinderDialog : public QDialog
{
    Q_OBJECT
public:
    DuplicateFinderDialog(const QString &folder, QWidget *parent);
};

/** Shows MD5, SHA-1 and SHA-256 of a file and checks them against a checksum that is pasted in. */
class ChecksumDialog : public QDialog
{
    Q_OBJECT
public:
    ChecksumDialog(const KFileItem &item, QWidget *parent);

private:
    void updateVerdict();

    QLabel *m_verdict;
    QLineEdit *m_expected;
    QLineEdit *m_values[3];
};

/** Renames the given files by a pattern, with a preview. */
class BatchRenameDialog : public QDialog
{
    Q_OBJECT
public:
    BatchRenameDialog(const KFileItemList &items, QWidget *parent);

    /** The new names for the current pattern, in the order of the items. */
    QStringList newNames() const;

private:
    void updatePreview();
    void rename();

    KFileItemList m_items;
    QLineEdit *m_pattern;
    QTableWidget *m_table;
    QDialogButtonBox *m_buttons;
};

/** A small window that shows what is in a file: a preview of an image, or the start of a text. */
class QuickViewDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QuickViewDialog(QWidget *parent);
    void show(const KFileItem &item);

private:
    FilePreviewWidget *m_preview;
};

#endif
