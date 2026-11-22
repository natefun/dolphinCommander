/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COMMANDERTOOLS_H
#define COMMANDERTOOLS_H

#include <KFileItem>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QHash>
#include <QList>
#include <QPointF>
#include <QStringList>
#include <QUrl>
#include <optional>

class QMimeData;

class FilePreviewWidget;
class QLabel;
class QKeyEvent;
class QPushButton;
class QMediaPlayer;
class QVideoWidget;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QCheckBox;
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

/** Saves the image at @p source next to itself in @p format ("png", "jpg", "webp", ...) and returns the new path, or an empty string if it failed. An existing
 * file is not overwritten. */
QString convertImage(const QString &source, const QString &format);

/** The name of a to-do file with its state flipped: .2do (open) becomes .2dx (done) and back. Other names stay as they are. */
QString toggledToDoName(const QString &fileName);

/** Writes the clipboard's image as a dated .png, or its text as a dated .txt, into @p folder, and returns the path (empty if the clipboard has neither or it
 * failed). */
QString saveClipboardAsFile(const QMimeData *clipboard, const QString &folder);

/** Where a photo was taken, from its metadata: x is the longitude and y the latitude. Empty if the file has no position. */
std::optional<QPointF> photoLocation(const QString &path);

/** A map of OpenStreetMap that shows @p location. */
QUrl mapUrl(const QPointF &location);

/** The total size in bytes of the files below @p folder, without following symlinks. */
qint64 folderSize(const QString &folder);

/** Groups of files below @p folder (recursively, without symlinks) that have the same content. Empty files are ignored. */
QList<QStringList> findDuplicates(const QString &folder);
}

/** Shows the size of each of the given folders, biggest first, as they are worked out. */
class FolderSizesDialog : public QDialog
{
    Q_OBJECT
public:
    FolderSizesDialog(const QStringList &folders, QWidget *parent);
};

/** Lists the duplicates below a folder and moves the ticked ones to the trash. */
class DuplicateFinderDialog : public QDialog
{
    Q_OBJECT
public:
    DuplicateFinderDialog(const QString &folder, QWidget *parent);
};

/**
 * A window to design a file view: which columns it shows, in what order and how wide they are.
 * The columns are the roles of the file model; the rows are in the order of the view, the shown ones first.
 */
class FileViewDesignerDialog : public QDialog
{
    Q_OBJECT
public:
    /** @p roles are the columns now shown, in order, and @p widths their widths (empty: automatic). */
    FileViewDesignerDialog(const QList<QByteArray> &roles, const QList<int> &widths, QWidget *parent);

    /** The columns that are ticked, in the order of the rows. */
    QList<QByteArray> roles() const;
    /** The width of each of roles(); empty when "Automatic widths" is ticked. */
    QList<int> widths() const;
    /** The name to keep the design under; empty if it is not to be saved. */
    QString name() const;
    /** True when the design is to be tried on the folder without keeping it (the "Apply Without Saving" button). */
    bool temporaryOnly() const;
    /** For each of roles(), the width of the view under which the column is hidden; 0 for the automatic rule. */
    QList<int> hideBelow() const;
    /** Sets the thresholds that the rows start with, by role. */
    void setHideBelow(const QHash<QByteArray, int> &thresholds);

private:
    void moveCurrentRow(int by);

    QTableWidget *m_table;
    QCheckBox *m_automatic;
    QLineEdit *m_name;
    bool m_temporaryOnly = false;
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
    /** The name of the file that is shown. */
    QString shownName() const;

    /** Moves the playing video or sound by @p milliseconds, within its length. Does nothing when no media is shown. */
    void seekBy(qint64 milliseconds);
    /** Steps one frame (about 40 ms) back or forth; the media is paused first. */
    void stepFrame(int direction);
    /** Plays or pauses. */
    void togglePlayback();
    /** The map to show where the photo was taken; empty when the file has no position. */
    QUrl mapLink() const;
    /** Where the media is, in milliseconds, and how long it is; both 0 when no media is shown. */
    qint64 mediaPosition() const;
    qint64 mediaDuration() const;

Q_SIGNALS:
    /** The Up (-1) or Down (+1) key asks for the previous or the next file. */
    void stepRequested(int delta);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    FilePreviewWidget *m_preview;
    QString m_shownName;
    QUrl m_mapLink;
    QPushButton *m_mapButton;
    QMediaPlayer *m_player;
    QVideoWidget *m_video;
};

#endif
