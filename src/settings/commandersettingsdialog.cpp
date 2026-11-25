/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commandersettingsdialog.h"

#include "commandersettings.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVBoxLayout>

using Kind = CommanderSettingsDialog::Kind;
using Support = CommanderSettingsDialog::Support;
using Choice = CommanderSettingsDialog::Choice;

namespace
{
QString windowsOnlyNote()
{
    return i18nc("@info:tooltip", "This setting is for Windows. There is nothing for it to change here.");
}
QString notYetNote()
{
    return i18nc("@info:tooltip", "This setting is kept, but the program does not act on it yet.");
}
}

QStringList CommanderSettingsDialog::tabs()
{
    return {i18nc("@title:tab", "General"),
            i18nc("@title:tab", "View"),
            i18nc("@title:tab", "Columns"),
            i18nc("@title:tab", "Preview"),
            i18nc("@title:tab", "Theme"),
            i18nc("@title:tab", "Window"),
            i18nc("@title:tab", "Advanced"),
            i18nc("@title:tab", "Other")};
}

const QList<CommanderSettingsDialog::Setting> &CommanderSettingsDialog::settings()
{
    static const QList<Setting> list = []() {
        QList<Setting> rows;
        const QStringList t = tabs();
        const QString general = t.at(0), view = t.at(1), columns = t.at(2), preview = t.at(3), theme = t.at(4), window = t.at(5), advanced = t.at(6),
                      other = t.at(7);
        auto add = [&rows](const QString &tab,
                           const QString &header,
                           const QString &key,
                           const QString &label,
                           Kind kind,
                           const QVariant &def,
                           Support support,
                           const QString &info = QString()) -> Setting & {
            Setting setting;
            setting.tab = tab;
            setting.header = header;
            setting.key = key;
            setting.label = label;
            setting.kind = kind;
            setting.defaultValue = def;
            setting.support = support;
            setting.info = info;
            rows.append(setting);
            return rows.last();
        };
        auto flag = [&](const QString &tab,
                        const QString &header,
                        const QString &key,
                        const QString &label,
                        bool def,
                        Support support,
                        const QString &info = QString()) {
            add(tab, header, key, label, Kind::Bool, def, support, info);
        };
        auto number = [&](const QString &tab,
                          const QString &header,
                          const QString &key,
                          const QString &label,
                          int def,
                          int min,
                          int max,
                          const QString &suffix,
                          Support support,
                          const QString &info = QString()) {
            Setting &s = add(tab, header, key, label, Kind::Int, def, support, info);
            s.minimum = min;
            s.maximum = max;
            s.suffix = suffix;
        };
        auto text = [&](const QString &tab,
                        const QString &header,
                        const QString &key,
                        const QString &label,
                        const QString &def,
                        Support support,
                        const QString &info = QString()) {
            add(tab, header, key, label, Kind::Text, def, support, info);
        };
        auto pick = [&](const QString &tab,
                        const QString &header,
                        const QString &key,
                        const QString &label,
                        const QList<Choice> &choices,
                        Support support,
                        const QString &info = QString()) {
            Setting &s = add(tab, header, key, label, Kind::Choice, choices.first().value, support, info);
            s.choices = choices;
        };
        auto button =
            [&](const QString &tab, const QString &header, const QString &key, const QString &label, Support support, const QString &info = QString()) {
                add(tab, header, key, label, Kind::Button, QVariant(), support, info);
            };
        const Support live = Support::Live, win = Support::WindowsOnly, not_yet = Support::NotYet;

        // ---- General ----
        const QString interaction = i18nc("@title:group", "Interaction");
        pick(general,
             interaction,
             QStringLiteral("OpeningItems"),
             i18nc("@label", "Opening items:"),
             {{QStringLiteral("double"), i18nc("@item:inlistbox", "Double-click opens items (single-click to select)")},
              {QStringLiteral("folders"), i18nc("@item:inlistbox", "Single click opens folders (Ctrl+click to select)")},
              {QStringLiteral("all"), i18nc("@item:inlistbox", "Single click also opens files")}},
             live);
        flag(general,
             interaction,
             QStringLiteral("UseDoubleClickToOpenParent"),
             i18nc("@option:check", "Double-Click in empty space to open parent folder"),
             true,
             live);
        flag(general,
             interaction,
             QStringLiteral("RenameItemsOnSecondClick"),
             i18nc("@option:check", "Rename items by clicking again without moving cursor"),
             false,
             live);
        flag(general,
             interaction,
             QStringLiteral("ShowWarningOnExtensionChange"),
             i18nc("@option:check", "Show warning dialog when changing extension during rename"),
             true,
             live);
        flag(general, interaction, QStringLiteral("ShowRenameSuggestions"), i18nc("@option:check", "Show rename suggestions popup"), false, not_yet);
        pick(general,
             interaction,
             QStringLiteral("RenameOnLostFocus"),
             i18nc("@label", "Clicking outside during rename operation should:"),
             {{QStringLiteral("rename"), i18nc("@item:inlistbox", "Rename")}, {QStringLiteral("cancel"), i18nc("@item:inlistbox", "Cancel rename")}},
             live);
        flag(general,
             interaction,
             QStringLiteral("PasteClipboardTextAsFile"),
             i18nc("@option:check", "Paste text and images as new files"),
             true,
             live,
             i18nc("@info", "Pressing Ctrl+V will create a text or image file with the clipboard content."));
        flag(general,
             interaction,
             QStringLiteral("ClipboardCheckText"),
             i18nc("@option:check", "Check and notify when there is text on clipboard"),
             false,
             live,
             i18nc("@info", "Shows notifications offering to create a file from text by pressing Ctrl+V."));
        pick(general,
             interaction,
             QStringLiteral("DragDropBehavior"),
             i18nc("@label", "Drag and Drop behavior:"),
             {{QStringLiteral("consistent"), i18nc("@item:inlistbox", "Consistent: Drag always moves; Ctrl+Drag always copies")},
              {QStringLiteral("inconsistent"), i18nc("@item:inlistbox", "Drag moves if the same drive, otherwise copies. Ctrl+Drag always copies")}},
             live);
        number(general,
               interaction,
               QStringLiteral("MouseDebounceTime"),
               i18nc("@label", "Double-Click threshold (ms)"),
               400,
               100,
               1500,
               i18nc("@item:valuesuffix", " ms"),
               live,
               i18nc("@info", "Reduces mouse button noise by setting a delay to ignore rapid, unintended clicks."));
        flag(general, interaction, QStringLiteral("UseSmoothScrolling"), i18nc("@option:check", "Smooth scrolling with mouse wheel"), true, live);
        flag(general, interaction, QStringLiteral("UseHighResScrolling"), i18nc("@option:check", "Use high-resolution touchpad scrolling"), true, not_yet);
        pick(general,
             interaction,
             QStringLiteral("ScrollingBehavior"),
             i18nc("@label", "Scrolling behavior:"),
             {{QStringLiteral("pixel"), i18nc("@item:inlistbox", "Pixel (smooth)")},
              {QStringLiteral("item"), i18nc("@item:inlistbox", "Item (snaps to item positions)")}},
             live);
        pick(general,
             interaction,
             QStringLiteral("DoubleClickArchive"),
             i18nc("@label", "Double-click on an archive should:"),
             {{QStringLiteral("popup"), i18nc("@item:inlistbox", "Show popup asking to extract or open in a default program")},
              {QStringLiteral("default"), i18nc("@item:inlistbox", "Open in default program")}},
             live);

        const QString tabsHeader = i18nc("@title:group", "Tabs");
        pick(general,
             tabsHeader,
             QStringLiteral("OnStart"),
             i18nc("@label", "On start:"),
             {{QStringLiteral("restore"), i18nc("@item:inlistbox", "Restore last used tabs")},
              {QStringLiteral("path"), i18nc("@item:inlistbox", "Create a single tab and open path:")}},
             live);
        text(general, tabsHeader, QStringLiteral("OnStartOpenPath"), i18nc("@label", "Path to open on start:"), QString(), live);
        pick(general,
             tabsHeader,
             QStringLiteral("NewTab"),
             i18nc("@label", "Creating a new tab should:"),
             {{QStringLiteral("duplicate"), i18nc("@item:inlistbox", "Duplicate current")}, {QStringLiteral("path"), i18nc("@item:inlistbox", "Open path:")}},
             live);
        text(general, tabsHeader, QStringLiteral("NewTabOpenPath"), i18nc("@label", "Path for a new tab:"), QString(), live);
        pick(general,
             tabsHeader,
             QStringLiteral("NewTabPlacement"),
             i18nc("@label", "Middle-click on a folder opens a new tab:"),
             {{QStringLiteral("after"), i18nc("@item:inlistbox", "After current")}, {QStringLiteral("end"), i18nc("@item:inlistbox", "At the end")}},
             live);
        flag(general,
             tabsHeader,
             QStringLiteral("SwitchToNewTab"),
             i18nc("@option:check", "Switch to newly created tab on middle-click on folder"),
             false,
             live);
        pick(general,
             tabsHeader,
             QStringLiteral("OpenArgsPath"),
             i18nc("@label", "Open paths from external source in:"),
             {{QStringLiteral("new"), i18nc("@item:inlistbox", "New tab")},
              {QStringLiteral("current"), i18nc("@item:inlistbox", "Current tab")},
              {QStringLiteral("existing"), i18nc("@item:inlistbox", "Existing tab with the same path")}},
             live);
        pick(general,
             tabsHeader,
             QStringLiteral("TabsShow"),
             i18nc("@label", "Tabs and favorites should show:"),
             {{QStringLiteral("name"), i18nc("@item:inlistbox", "Folder only (Foldername)")},
              {QStringLiteral("full"), i18nc("@item:inlistbox", "Full path (/home/.../Foldername)")}},
             live);

        const QString historyHeader = i18nc("@title:group", "History");
        number(general, historyHeader, QStringLiteral("HistoryMaxItems"), i18nc("@label", "Max number of paths saved"), 20, 1, 200, QString(), live);
        flag(general, historyHeader, QStringLiteral("ClearHistoryOnClose"), i18nc("@option:check", "Clear History on close"), false, live);
        flag(general,
             historyHeader,
             QStringLiteral("RemoveDuplicatesFromTabsHistory"),
             i18nc("@option:check", "Remove duplicates from tab's path history"),
             true,
             not_yet);

        // ---- View ----
        const QString files = i18nc("@title:group", "Files view");
        pick(view,
             files,
             QStringLiteral("FilesViewDefault"),
             i18nc("@label", "Default files view"),
             {{QStringLiteral("details"), i18nc("@item:inlistbox", "Details")},
              {QStringLiteral("compact"), i18nc("@item:inlistbox", "Compact")},
              {QStringLiteral("icons"), i18nc("@item:inlistbox", "Thumbnail file view")}},
             live);
        flag(view,
             files,
             QStringLiteral("InheritFilesView"),
             i18nc("@option:check", "Inherit files view"),
             false,
             live,
             i18nc("@info", "Files view and sorting will be inherited from the parent folder."));
        flag(view, files, QStringLiteral("ShowFilesViewToolbar"), i18nc("@option:check", "Show Files View toolbar buttons"), true, live);
        flag(view,
             files,
             QStringLiteral("AutoThumbsDetails"),
             i18nc("@option:check", "Auto-switch to thumbnail view"),
             false,
             live,
             i18nc("@info", "The program will check the first few files in a folder and if most are images it switches to the thumbnail view."));
        flag(view,
             files,
             QStringLiteral("UseSingleNewItemButton"),
             i18nc("@option:check", "Show a single New Item button instead of 3 buttons for New File/Folder/Note"),
             false,
             not_yet);

        const QString sorting = i18nc("@title:group", "Sorting");
        pick(view,
             sorting,
             QStringLiteral("SortItemsBy"),
             i18nc("@label", "Sort items by"),
             {{QStringLiteral("name"), i18nc("@item:inlistbox", "Name")},
              {QStringLiteral("type"), i18nc("@item:inlistbox", "Type")},
              {QStringLiteral("size"), i18nc("@item:inlistbox", "Size")},
              {QStringLiteral("modified"), i18nc("@item:inlistbox", "Date modified")},
              {QStringLiteral("created"), i18nc("@item:inlistbox", "Date created")}},
             live,
             i18nc("@info", "These settings won't affect folders that already have sorting saved."));
        pick(view,
             sorting,
             QStringLiteral("SortDirection"),
             i18nc("@label", "Sort direction"),
             {{QStringLiteral("ascending"), i18nc("@item:inlistbox", "Ascending")}, {QStringLiteral("descending"), i18nc("@item:inlistbox", "Descending")}},
             live);
        flag(view, sorting, QStringLiteral("SortKeepFoldersOnTop"), i18nc("@option:check", "Keep folders above files"), true, live);
        flag(view,
             sorting,
             QStringLiteral("ShowKeepFoldersOnTopToggle"),
             i18nc("@option:check", "Show toggle for listing folders above files"),
             false,
             not_yet);
        flag(view,
             sorting,
             QStringLiteral("FoldersSortedByName"),
             i18nc("@option:check", "Sort folders by name always"),
             false,
             live,
             i18nc("@info", "Even if you sort files by date, folders will remain sorted by name."));
        pick(view,
             sorting,
             QStringLiteral("LiveSort"),
             i18nc("@label", "After renaming or updating file:"),
             {{QStringLiteral("sort"), i18nc("@item:inlistbox", "Sort with other files")},
              {QStringLiteral("keep"), i18nc("@item:inlistbox", "Keep file location in the list")}},
             not_yet);

        const QString visibility = i18nc("@title:group", "Visibility");
        flag(view,
             visibility,
             QStringLiteral("ShowHidden"),
             i18nc("@option:check", "Show hidden files and folders"),
             false,
             live,
             i18nc("@info", "Global shortcut: Alt+. or Ctrl+H"));
        flag(view, visibility, QStringLiteral("ShowSystem"), i18nc("@option:check", "Show protected system files and folders"), false, win);
        flag(view,
             visibility,
             QStringLiteral("ShowTemporary"),
             i18nc("@option:check", "Show temporary files"),
             false,
             live,
             i18nc("@info", "Shows .tmp files, files that start with ~$, and files with the temporary attribute."));
        flag(view, visibility, QStringLiteral("HideTemporary"), i18nc("@option:check", "Hide temporary files"), false, live);
        flag(view, visibility, QStringLiteral("TreatDotFilesAsHidden"), i18nc("@option:check", "Files starting with dot"), true, live);
        text(view, visibility, QStringLiteral("HiddenFilesPrefixList"), i18nc("@label", "Prefixed files"), QString(), live);
        text(view, visibility, QStringLiteral("HiddenFilesList"), i18nc("@label", "Full names"), QString(), live);
        text(view,
             visibility,
             QStringLiteral("HiddenFilesExtList"),
             i18nc("@label", "Extensions"),
             QString(),
             live,
             i18nc("@info", "Visibility is controlled by the checkbox above or the shortcut."));
        flag(view, visibility, QStringLiteral("ShowFileExtension"), i18nc("@option:check", "Show file extensions"), true, live);
        flag(view,
             visibility,
             QStringLiteral("HideExtensionIfExtensionColumnVisible"),
             i18nc("@option:check", "Hide file extension after filename if extension column is visible"),
             false,
             live);
        flag(view, visibility, QStringLiteral("ShowFolderSize"), i18nc("@option:check", "Show folder sizes"), false, live);
        flag(view,
             visibility,
             QStringLiteral("ShowFolderSizeOnNetwork"),
             i18nc("@option:check", "Show Folder size on network folders"),
             false,
             not_yet,
             i18nc("@info", "Off by default as it makes browsing network folders slower."));
        flag(view,
             visibility,
             QStringLiteral("ShowMetadataOnNetwork"),
             i18nc("@option:check", "Show Metadata on network files"),
             false,
             not_yet,
             i18nc("@info", "Off by default as it makes browsing network folders slower."));
        flag(view,
             visibility,
             QStringLiteral("ShowFileAttributes"),
             i18nc("@option:check", "Show file attributes and synchronization state icons"),
             false,
             win);
        flag(view, visibility, QStringLiteral("SkipSearchingReparsePoints"), i18nc("@option:check", "Skip searching within reparse points"), false, win);
        text(view,
             visibility,
             QStringLiteral("ExcludedWatcherKeywords"),
             i18nc("@label", "Excluded watcher keywords"),
             QString(),
             not_yet,
             i18nc("@info", "Changes in files with full paths containing these strings won't be reported."));

        const QString itemProps = i18nc("@title:group", "Item properties");
        pick(view,
             itemProps,
             QStringLiteral("DateShown"),
             i18nc("@label", "Date shown in list of files"),
             {{QStringLiteral("modified"), i18nc("@item:inlistbox", "Date modified")}, {QStringLiteral("created"), i18nc("@item:inlistbox", "Date created")}},
             live);
        pick(view,
             itemProps,
             QStringLiteral("DateFormat"),
             i18nc("@label", "Date format"),
             {{QStringLiteral("system"), i18nc("@item:inlistbox", "Default")},
              {QStringLiteral("iso"), QStringLiteral("2026-11-24")},
              {QStringLiteral("long"), i18nc("@item:inlistbox", "Long date")}},
             live);
        pick(view,
             itemProps,
             QStringLiteral("TimeFormat"),
             i18nc("@label", "Time format"),
             {{QStringLiteral("system"), i18nc("@item:inlistbox", "Default")},
              {QStringLiteral("24h"), QStringLiteral("24h")},
              {QStringLiteral("12h"), i18nc("@item:inlistbox", "12h with AM/PM")}},
             live);
        pick(view,
             itemProps,
             QStringLiteral("FileAge"),
             i18nc("@label", "File age / relative date"),
             {{QStringLiteral("color"), i18nc("@item:inlistbox", "Color")}, {QStringLiteral("none"), i18nc("@item:inlistbox", "None")}},
             live);
        flag(view,
             itemProps,
             QStringLiteral("AgeStripes"),
             i18nc("@option:check", "Mark recent files"),
             false,
             live,
             i18nc("@info", "Shows a color indicator on files from the current and last week."));
        flag(view, itemProps, QStringLiteral("ColoredBadges"), i18nc("@option:check", "Make size over 1 GB more prominent with a color"), true, live);
        pick(view,
             itemProps,
             QStringLiteral("SizeFormat"),
             i18nc("@label", "Size format"),
             {{QStringLiteral("auto"), i18nc("@item:inlistbox", "Automatic")}, {QStringLiteral("mb"), QStringLiteral("MB")}},
             live);
        flag(view,
             itemProps,
             QStringLiteral("UseOldDefinitionOfMegabyte"),
             i18nc("@option:check", "Use old definition of Megabyte (as in Windows Explorer)"),
             false,
             live);
        pick(view,
             itemProps,
             QStringLiteral("ItemBackgroundColorSource"),
             i18nc("@label", "Item coloring"),
             {{QStringLiteral("none"), i18nc("@item:inlistbox", "None")}, {QStringLiteral("tag"), i18nc("@item:inlistbox", "Color tag")}},
             not_yet);
        number(view,
               itemProps,
               QStringLiteral("FileNameMaxLines"),
               i18nc("@label", "File name maximum lines"),
               2,
               1,
               10,
               QString(),
               live,
               i18nc("@info", "For the thumbnail view. Only when the thumbnail is larger than 96 pixels."));
        pick(view,
             itemProps,
             QStringLiteral("FileNameTrimming"),
             i18nc("@label", "File name trimming"),
             {{QStringLiteral("end"), i18nc("@item:inlistbox", "End")}, {QStringLiteral("middle"), i18nc("@item:inlistbox", "Middle")}},
             live,
             i18nc("@info", "Determines where the file name will be trimmed when it cannot fit the available space."));
        pick(view,
             itemProps,
             QStringLiteral("GridViewFileNamePosition"),
             i18nc("@label", "File name position in Grid view"),
             {{QStringLiteral("bottom"), i18nc("@item:inlistbox", "Bottom")}, {QStringLiteral("right"), i18nc("@item:inlistbox", "Right")}},
             not_yet);
        number(view, itemProps, QStringLiteral("MetadataLineCount"), i18nc("@label", "Lines of text"), 2, 0, 6, QString(), not_yet);
        text(view,
             itemProps,
             QStringLiteral("MetadataExcerpts"),
             i18nc("@label", "Show text excerpts for these file types"),
             QStringLiteral("txt,md"),
             not_yet,
             i18nc("@info", "Comma separated; no '.'; example: txt,bat,md"));
        flag(view,
             itemProps,
             QStringLiteral("LimitFileNameLength"),
             i18nc("@option:check", "Limit file name length for maximum compatibility with other software"),
             false,
             live);
        number(view, itemProps, QStringLiteral("MaximumFilenameLength"), i18nc("@label", "Maximum file name length"), 255, 16, 255, QString(), live);
        flag(view,
             itemProps,
             QStringLiteral("ShowFolderBackgroundImage"),
             i18nc("@option:check", "Show folder background image"),
             true,
             live,
             i18nc("@info", "If a folder contains a file named folder.jpg, cover.jpg or background.jpg it is shown behind the list."));

        const QString thumbs = i18nc("@title:group", "Thumbnails");
        pick(view,
             thumbs,
             QStringLiteral("FolderThumbnailStyle"),
             i18nc("@label", "Folder thumbnail style"),
             {{QStringLiteral("icon"), i18nc("@item:inlistbox", "Folder icon")},
              {QStringLiteral("content"), i18nc("@item:inlistbox", "Pictures from the folder")}},
             not_yet);
        text(view,
             thumbs,
             QStringLiteral("ThumbnailExcludedExtensions"),
             i18nc("@label", "File extensions to exclude thumbnails"),
             QString(),
             live,
             i18nc("@info", "Comma separated; no spaces."));
        button(view,
               thumbs,
               QStringLiteral("ClearThumbnailCache"),
               i18nc("@action:button", "Clear thumbnail cache"),
               live,
               i18nc("@info", "Deletes the saved thumbnails. They are made again when needed."));
        flag(view, thumbs, QStringLiteral("UseLocalThumbsDb"), i18nc("@option:check", "Use local thumbnails database (faster)"), false, win);
        flag(view, thumbs, QStringLiteral("UsePDFThumbs"), i18nc("@option:check", "Custom .pdf and .ai thumbnail extraction"), false, win);
        flag(view, thumbs, QStringLiteral("GetCloudThumbs"), i18nc("@option:check", "Get thumbnails of cloud files"), false, win);
        flag(view, thumbs, QStringLiteral("RegisterMaxThumbs"), i18nc("@option:check", "Register 3ds Max thumbnail handler"), false, win);
        flag(view,
             thumbs,
             QStringLiteral("RegistryOfficeThumbs"),
             i18nc("@option:check", "Force Microsoft 365/Office to save thumbnails in files"),
             false,
             win);

        const QString icons = i18nc("@title:group", "Icons");
        text(view, icons, QStringLiteral("FileIconsPack"), i18nc("@label", "File icons pack"), QString(), not_yet);
        text(view, icons, QStringLiteral("FolderIconsPack"), i18nc("@label", "Folder icons pack"), QString(), not_yet);
        text(view, icons, QStringLiteral("FolderIconOverride"), i18nc("@label", "Override all folder icons with:"), QString(), live);
        button(view, icons, QStringLiteral("DownloadIconPacks"), i18nc("@action:button", "Download icon packs"), win);

        // ---- Columns ----
        const QString widths = i18nc("@title:group", "Pane sizes");
        number(columns,
               widths,
               QStringLiteral("ColumnMinimumWidth"),
               i18nc("@label", "Columns minimum width"),
               80,
               40,
               400,
               QStringLiteral(" px"),
               live,
               i18nc("@info", "Minimum width for all columns."));
        number(columns,
               widths,
               QStringLiteral("ColumnWidthSmall"),
               i18nc("@label", "Columns small width"),
               150,
               40,
               400,
               QStringLiteral(" px"),
               live,
               i18nc("@info", "If there isn't enough space, left-most columns will contract up to this width."));
        number(columns,
               widths,
               QStringLiteral("ColumnParentWidth"),
               i18nc("@label", "Parent column width"),
               170,
               40,
               600,
               QStringLiteral(" px"),
               live,
               i18nc("@info", "When space is limited, the parent folder column will try to expand to this size."));
        number(columns,
               widths,
               QStringLiteral("ColumnWidthCurrentStandardLayout"),
               i18nc("@label", "Current (rightmost) column pane width in Standard layout"),
               300,
               80,
               1000,
               QStringLiteral(" px"),
               not_yet);
        number(columns,
               widths,
               QStringLiteral("ColumnWidthCurrentColumnsLayout"),
               i18nc("@label", "Current (rightmost) column pane width in Columns layout"),
               170,
               80,
               1000,
               QStringLiteral(" px"),
               live);
        number(columns,
               widths,
               QStringLiteral("ColumnWidthOther"),
               i18nc("@label", "Maximum non-current column width"),
               300,
               80,
               1000,
               QStringLiteral(" px"),
               live);
        const QString alignment = i18nc("@title:group", "Alignment");
        const QList<Choice> aligns = {{QStringLiteral("left"), i18nc("@item:inlistbox", "Left")},
                                      {QStringLiteral("right"), i18nc("@item:inlistbox", "Right")},
                                      {QStringLiteral("center"), i18nc("@item:inlistbox", "Center")}};
        pick(columns, alignment, QStringLiteral("ColumnAlignStandardLayout"), i18nc("@label", "Column Pane alignment in Standard Layout"), aligns, not_yet);
        pick(columns, alignment, QStringLiteral("ColumnAlignColumnsLayout"), i18nc("@label", "Column Pane alignment in Columns Layout"), aligns, not_yet);
        const QString expand = i18nc("@title:group", "Expanding");
        flag(columns,
             expand,
             QStringLiteral("ColumnAutoExpandStandardLayout"),
             i18nc("@option:check", "Expand Column when cursor enters it (Standard layout)"),
             false,
             not_yet);
        flag(columns,
             expand,
             QStringLiteral("ColumnAutoExpandColumnsLayout"),
             i18nc("@option:check", "Expand Column when cursor enters it (Columns layout)"),
             false,
             not_yet);
        number(columns,
               expand,
               QStringLiteral("ColumnAdvancedExpandTriggerWidth"),
               i18nc("@label", "Trigger width"),
               150,
               40,
               600,
               QStringLiteral(" px"),
               not_yet,
               i18nc("@info", "Only columns smaller than this are expanded."));
        number(columns,
               expand,
               QStringLiteral("ColumnAdvancedDelay"),
               i18nc("@label", "Expand delay (ms)"),
               300,
               0,
               3000,
               QStringLiteral(" ms"),
               not_yet,
               i18nc("@info", "When the cursor enters a folder column, the program waits for this long."));
        number(columns,
               expand,
               QStringLiteral("ColumnAdvancedDuration"),
               i18nc("@label", "Expand duration (ms)"),
               200,
               0,
               3000,
               QStringLiteral(" ms"),
               not_yet,
               i18nc("@info", "and then animates expanding the column for this long."));
        flag(columns,
             expand,
             QStringLiteral("CreateNewColumnsWithoutAnimating"),
             i18nc("@option:check", "Create New Columns without animating"),
             false,
             not_yet);

        // ---- Preview ----
        const QString previewWindow = i18nc("@title:group", "Preview window");
        pick(preview,
             previewWindow,
             QStringLiteral("PreviewWindow"),
             i18nc("@label", "Preview window"),
             {{QStringLiteral("builtin"), i18nc("@item:inlistbox", "Built-in preview")},
              {QStringLiteral("quicklook"), i18nc("@item:inlistbox", "QuickLook (must be manually installed)")}},
             win);
        flag(preview,
             previewWindow,
             QStringLiteral("PreviewShortcut"),
             i18nc("@option:check", "Pressing Spacebar key opens file preview window"),
             true,
             live,
             i18nc("@info", "(Pressing Esc or Spacebar again closes it)"));
        flag(preview, previewWindow, QStringLiteral("QuickViewAutoplay"), i18nc("@option:check", "Autoplay video/audio files in preview window"), false, live);
        pick(preview,
             previewWindow,
             QStringLiteral("QuickViewImageScale"),
             i18nc("@label", "Display small images"),
             {{QStringLiteral("original"), i18nc("@item:inlistbox", "Original size")},
              {QStringLiteral("fit"), i18nc("@item:inlistbox", "Scaled to fit preview pane")}},
             live);
        pick(preview,
             previewWindow,
             QStringLiteral("QuickViewSmoothChoice"),
             i18nc("@label", "Image scaling interpolation"),
             {{QStringLiteral("smooth"), i18nc("@item:inlistbox", "Smooth (better quality)")},
              {QStringLiteral("pixelated"), i18nc("@item:inlistbox", "Pixelated (better performance)")}},
             live);
        number(preview,
               previewWindow,
               QStringLiteral("PreviewTextMaxLength"),
               i18nc("@label", "Preview text length"),
               100,
               1,
               10000,
               QStringLiteral(" K"),
               live,
               i18nc("@info", "Number of characters (in thousands) to read from a file to show in the preview."));
        flag(preview, previewWindow, QStringLiteral("PreviewDownloadsCloudFiles"), i18nc("@option:check", "Download cloud files when previewing"), false, win);
        flag(preview, previewWindow, QStringLiteral("EnableSFP"), i18nc("@option:check", "Enable Explorer Preview (Shell File Preview)"), false, win);
        pick(preview,
             previewWindow,
             QStringLiteral("MediaJumpUnit"),
             i18nc("@label", "Left and Right arrow control for media playback"),
             {{QStringLiteral("seconds"), i18nc("@item:inlistbox", "Specific number of seconds")},
              {QStringLiteral("percent"), i18nc("@item:inlistbox", "Percentage of total length")}},
             live);
        number(preview, previewWindow, QStringLiteral("MediaJumpPercent"), i18nc("@label", "Percent jump"), 12, 1, 50, QStringLiteral(" %"), live);
        number(preview, previewWindow, QStringLiteral("MediaJumpSecondsBack"), i18nc("@label", "Rewind seconds"), 5, 1, 600, QStringLiteral(" s"), live);
        number(preview, previewWindow, QStringLiteral("MediaJumpSecondsForward"), i18nc("@label", "Forward seconds"), 10, 1, 600, QStringLiteral(" s"), live);
        const QString paneHeader = i18nc("@title:group", "Preview pane");
        pick(preview,
             paneHeader,
             QStringLiteral("PhotoLocation"),
             i18nc("@label", "Photo location button opens web browser with:"),
             {{QStringLiteral("osm"), QStringLiteral("OpenStreetMap")},
              {QStringLiteral("google"), i18nc("@item:inlistbox", "Google Maps")},
              {QStringLiteral("bing"), i18nc("@item:inlistbox", "Bing Maps")}},
             live);

        // ---- Theme ----
        const QString themeHeader = i18nc("@title:group", "Theme");
        pick(theme,
             themeHeader,
             QStringLiteral("ColorTheme"),
             i18nc("@label", "Theme"),
             {{QStringLiteral("system"), i18nc("@item:inlistbox", "Use system theme")},
              {QStringLiteral("light"), i18nc("@item:inlistbox", "Light")},
              {QStringLiteral("dark"), i18nc("@item:inlistbox", "Dark")},
              {QStringLiteral("nord"), i18nc("@item:inlistbox", "Nord")}},
             live);
        text(theme,
             themeHeader,
             QStringLiteral("AccentColor"),
             i18nc("@label", "Accent Color"),
             QString(),
             live,
             i18nc("@info", "(HEX value in #RRGGBB format)"));
        flag(theme, themeHeader, QStringLiteral("UseSystemAccentColor"), i18nc("@option:check", "Use system accent color"), false, win);
        flag(theme, themeHeader, QStringLiteral("OptUseThinScrollbar"), i18nc("@option:check", "Use thin scrollbar"), false, live);
        const QString ui = i18nc("@title:group", "UI properties");
        text(theme, ui, QStringLiteral("FontFamily"), i18nc("@label", "Font"), QString(), live, i18nc("@info", "Leave empty for the font of the desktop."));
        number(theme,
               ui,
               QStringLiteral("FontPointSize"),
               i18nc("@label", "Font size"),
               0,
               0,
               24,
               QStringLiteral(" pt"),
               live,
               i18nc("@info", "0 is the size of the desktop."));
        pick(theme,
             ui,
             QStringLiteral("FontStretch"),
             i18nc("@label", "Font stretch"),
             {{QStringLiteral("normal"), i18nc("@item:inlistbox", "Normal")},
              {QStringLiteral("contracted"), i18nc("@item:inlistbox", "Contracted")},
              {QStringLiteral("expanded"), i18nc("@item:inlistbox", "Expanded")}},
             live);
        number(theme, ui, QStringLiteral("Padding"), i18nc("@label", "Padding"), 0, 0, 20, QStringLiteral(" px"), live);
        number(theme,
               ui,
               QStringLiteral("ContextMenuItemPadding"),
               i18nc("@label", "Context Menu Item padding"),
               0,
               0,
               20,
               QStringLiteral(" px"),
               not_yet,
               i18nc("@info", "Make it larger for easier touchscreen access."));
        flag(theme, ui, QStringLiteral("EnableAnimations"), i18nc("@option:check", "Enable Animations"), true, live);
        number(theme, ui, QStringLiteral("AnimationDuration"), i18nc("@label", "Animation duration (ms)"), 200, 0, 1000, QStringLiteral(" ms"), live);
        pick(theme,
             ui,
             QStringLiteral("FocusedElementStyle"),
             i18nc("@label", "Keyboard-focused element display style"),
             {{QStringLiteral("default"), i18nc("@item:inlistbox", "Default")}, {QStringLiteral("none"), i18nc("@item:inlistbox", "None")}},
             not_yet);
        flag(theme, ui, QStringLiteral("UseAcrylicWindow"), i18nc("@option:check", "Use Acrylic window"), false, win);
        flag(theme, ui, QStringLiteral("UseMicaWindow"), i18nc("@option:check", "Use Mica window effect"), false, win);
        flag(theme, ui, QStringLiteral("UseAcrylicMenu"), i18nc("@option:check", "Use acrylic effect in menu"), false, win);
        flag(theme, ui, QStringLiteral("UseLayoutRounding"), i18nc("@option:check", "Pixel-perfect layout"), false, win);
        pick(theme,
             ui,
             QStringLiteral("TextRendering"),
             i18nc("@label", "Text rendering"),
             {{QStringLiteral("auto"), i18nc("@item:inlistbox", "Default")}},
             win);
        pick(theme,
             ui,
             QStringLiteral("Rendering"),
             i18nc("@label", "Rendering"),
             {{QStringLiteral("hardware"), i18nc("@item:inlistbox", "Hardware (DirectX)")}, {QStringLiteral("software"), i18nc("@item:inlistbox", "Software")}},
             win);

        // ---- Window ----
        const QString layoutHeader = i18nc("@title:group", "Window layout");
        pick(window,
             layoutHeader,
             QStringLiteral("Layout"),
             i18nc("@label", "Window layout"),
             {{QStringLiteral("0"), i18nc("@item:inlistbox", "Standard")},
              {QStringLiteral("1"), i18nc("@item:inlistbox", "Columns")},
              {QStringLiteral("2"), i18nc("@item:inlistbox", "Classic")}},
             live);
        const QString sidebar = i18nc("@title:group", "Sidebar");
        flag(window, sidebar, QStringLiteral("ShowDrivesCard"), i18nc("@option:check", "Show drives in sidebar"), true, live);
        flag(window, sidebar, QStringLiteral("ShowNetworksInSidebar"), i18nc("@option:check", "Show Networks group in sidebar"), true, live);
        flag(window, sidebar, QStringLiteral("ShowHistoryInSidebar"), i18nc("@option:check", "Show History pane in sidebar"), true, live);
        flag(window,
             sidebar,
             QStringLiteral("ShowScriptsButton"),
             i18nc("@option:check", "Show Scripts button"),
             false,
             live,
             i18nc("@info", "Know the scripts you are running! Security risk!"));
        button(window,
               sidebar,
               QStringLiteral("CreateDefaultSidebarFolders"),
               i18nc("@action:button", "Create default sidebar folders"),
               live,
               i18nc("@info", "Creates a favorites group and adds the folders of your home: Desktop, Documents, Downloads, Pictures, Music and Videos."));
        pick(window,
             sidebar,
             QStringLiteral("ComputerNameQuestion"),
             i18nc("@label", "Root folder alias"),
             {{QStringLiteral("machine"), i18nc("@item:inlistbox", "Machine name")}, {QStringLiteral("custom"), i18nc("@item:inlistbox", "Custom alias")}},
             live,
             i18nc("@info", "The name of the first column pane that contains the drives list."));
        text(window, sidebar, QStringLiteral("ComputerNameCustom"), i18nc("@label", "Custom alias"), QString(), live);
        flag(window,
             sidebar,
             QStringLiteral("ShowEjectOnSubstDrives"),
             i18nc("@option:check", "Show Eject button on Subst drives (folders mounted as drives)"),
             false,
             win);
        flag(window, sidebar, QStringLiteral("ImportFavoritesFromV2"), i18nc("@option:check", "Import Favorites from V2"), false, win);
        const QString windowHeader = i18nc("@title:group", "Window");
        flag(window,
             windowHeader,
             QStringLiteral("KeepRunningWhenClosed"),
             i18nc("@option:check", "Keep running in background (Close to System Tray)"),
             false,
             live,
             i18nc("@info", "If on, the program stays in the system tray for a faster next start."));
        flag(window,
             windowHeader,
             QStringLiteral("HideLastWindow"),
             i18nc("@option:check", "Hide last window instead of close"),
             false,
             live,
             i18nc("@info", "Opens the window instantly but uses more memory."));
        flag(window, windowHeader, QStringLiteral("Autostart"), i18nc("@option:check", "Autostart"), false, live, i18nc("@info", "Run when you log in."));
        flag(window, windowHeader, QStringLiteral("ShowWindowNameInTaskbar"), i18nc("@option:check", "Show window name in Taskbar"), false, win);
        pick(window,
             windowHeader,
             QStringLiteral("ShortcutClick"),
             i18nc("@label", "If program has a window open, double-clicking its shortcut should:"),
             {{QStringLiteral("switch"), i18nc("@item:inlistbox", "Switch to the existing window")},
              {QStringLiteral("create"), i18nc("@item:inlistbox", "Prompt to create another window")}},
             win);

        // ---- Advanced ----
        const QString advancedHeader = i18nc("@title:group", "Advanced");
        button(advanced, advancedHeader, QStringLiteral("ShortcutEditor"), i18nc("@action:button", "Shortcuts Editor"), live);
        text(advanced,
             advancedHeader,
             QStringLiteral("CommandShell"),
             i18nc("@label", "Command shell"),
             QString(),
             live,
             i18nc("@info", "Exe path and arguments. $CurrentPath will be replaced with the current path."));
        pick(advanced,
             advancedHeader,
             QStringLiteral("NotificationLevel"),
             i18nc("@label", "Notification level"),
             {{QStringLiteral("all"), i18nc("@item:inlistbox", "All")}, {QStringLiteral("errors"), i18nc("@item:inlistbox", "Errors only")}},
             live);
        flag(advanced,
             advancedHeader,
             QStringLiteral("ShowScriptRunConfirmationDialog"),
             i18nc("@option:check", "Show a dialog to confirm running script"),
             false,
             live);
        text(advanced,
             advancedHeader,
             QStringLiteral("VideoDownloadParameters"),
             i18nc("@label", "Download videos by pasting URL using following parameters"),
             QString(),
             live,
             i18nc("@info", "yt-dlp must be installed. The command must contain both $CurrentDir and $URL."));
        text(advanced,
             advancedHeader,
             QStringLiteral("AudioDownloadParameters"),
             i18nc("@label", "Download Audio by pasting URL using following parameters"),
             QString(),
             live);
        flag(advanced, advancedHeader, QStringLiteral("LogDetailedMessages"), i18nc("@option:check", "Log detailed messages"), false, live);
        flag(advanced, advancedHeader, QStringLiteral("ShowErrorReportDialog"), i18nc("@option:check", "Show error report dialog on crash"), false, win);
        flag(advanced, advancedHeader, QStringLiteral("UseOldDragAndDropSystem"), i18nc("@option:check", "Use old Drag&Drop system"), false, win);
        flag(advanced,
             advancedHeader,
             QStringLiteral("FixStylusLag"),
             i18nc("@option:check", "Fix Stylus lag (breaks touchscreen double-tap to open)"),
             false,
             win);
        pick(advanced,
             advancedHeader,
             QStringLiteral("FileOperationsHandler"),
             i18nc("@label", "File operations handler"),
             {{QStringLiteral("default"), i18nc("@item:inlistbox", "Default")}},
             win);
        const QString shell = i18nc("@title:group", "Context Menu");
        flag(advanced,
             shell,
             QStringLiteral("UseShellContextMenuSyncCascadeMenu"),
             i18nc("@option:check", "Wait for all items before showing context menu"),
             false,
             win);
        flag(advanced,
             shell,
             QStringLiteral("ShowOverlayOnShellContextMenu"),
             i18nc("@option:check", "Block window when Shell Context Menu is open"),
             false,
             win);
        flag(advanced, shell, QStringLiteral("GetRecentMenuItems"), i18nc("@option:check", "Get recent menu items"), false, win);
        flag(advanced, shell, QStringLiteral("OpenOfficeFilesThroughExplorer"), i18nc("@option:check", "Open Office files through Explorer"), false, win);
        text(advanced, shell, QStringLiteral("ExcludedShellContextMenuItems"), i18nc("@label", "Excluded Shell Context Menu items"), QString(), win);
        const QString experimental = i18nc("@title:group", "Experimental");
        button(advanced,
               experimental,
               QStringLiteral("DefaultFileManagerRegister"),
               i18nc("@action:button", "Register as a default file manager"),
               live,
               i18nc("@info", "Makes this program the one that opens folders."));
        flag(advanced, experimental, QStringLiteral("WinERegister"), i18nc("@option:check", "Register Win+E hotkey"), false, win);
        flag(advanced,
             experimental,
             QStringLiteral("HotkeyVizor"),
             i18nc("@option:check", "Use global hotkey Ctrl+` to show docked program window"),
             true,
             not_yet);
        flag(advanced, experimental, QStringLiteral("DialogConnector"), i18nc("@option:check", "Dialog connector"), false, win);

        // ---- Other ----
        const QString misc = i18nc("@title:group", "Other");
        button(other, misc, QStringLiteral("OpenSettingsFolder"), i18nc("@action:button", "Settings location"), live);
        button(other,
               misc,
               QStringLiteral("RestoreDefault"),
               i18nc("@action:button", "Restore to default"),
               live,
               i18nc("@info", "Puts every setting of this window back to what it was at the start."));
        text(other,
             misc,
             QStringLiteral("ExtensionsExecutable"),
             i18nc("@label", "File types (executable)"),
             QString(),
             not_yet,
             i18nc("@info", "File types that will accept a file to be dropped onto."));
        flag(other, misc, QStringLiteral("UpdateToVersionAhead"), i18nc("@option:check", "Update to version ahead (less stable)"), false, win);
        flag(other,
             misc,
             QStringLiteral("UseOneCommanderUpdater"),
             i18nc("@option:check", "Use external updater program instead of PowerShell script"),
             false,
             win);
        flag(other, misc, QStringLiteral("AddToEnvironmentPath"), i18nc("@option:check", "Add to user PATH variable"), false, win);
        flag(other, misc, QStringLiteral("CreatePSAlias"), i18nc("@option:check", "Create PowerShell alias 'oc'"), false, win);
        flag(other, misc, QStringLiteral("RegistryAddOpenInOC"), i18nc("@option:check", "Add \"Open in OneCommander\" context menu"), false, win);
        text(other, misc, QStringLiteral("EmailNamingFormat"), i18nc("@label", "Email naming format (drag from Outlook)"), QString(), win);
        text(other, misc, QStringLiteral("OneDriveFolderPath"), i18nc("@label", "Path to OneDrive folder"), QString(), win);
        return rows;
    }();
    return list;
}

CommanderSettingsDialog::CommanderSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Settings"));
    setObjectName(QStringLiteral("commanderSettingsDialog"));
    resize(760, 640);
    build();
}

void CommanderSettingsDialog::build()
{
    auto *layout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    layout->addWidget(m_tabs, 1);

    const QStringList tabNames = tabs();
    QList<QVBoxLayout *> pages;
    for (const QString &name : tabNames) {
        auto *scroll = new QScrollArea(m_tabs);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto *content = new QWidget(scroll);
        auto *pageLayout = new QVBoxLayout(content);
        pageLayout->setContentsMargins(12, 8, 12, 8);
        pageLayout->setSpacing(6);
        scroll->setWidget(content);
        m_tabs->addTab(scroll, name);
        pages << pageLayout;
    }

    QString lastHeader;
    int lastTab = -1;
    for (const Setting &setting : settings()) {
        const int tabIndex = int(tabNames.indexOf(setting.tab));
        QVBoxLayout *page = pages.at(tabIndex);
        if (tabIndex != lastTab || setting.header != lastHeader) {
            auto *header = new QLabel(setting.header, page->parentWidget());
            QFont font = header->font();
            font.setBold(true);
            font.setPointSizeF(font.pointSizeF() * 1.2);
            header->setFont(font);
            header->setContentsMargins(0, 12, 0, 0);
            header->setObjectName(QStringLiteral("settingsHeader"));
            page->addWidget(header);
            auto *line = new QFrame(page->parentWidget());
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            page->addWidget(line);
            lastHeader = setting.header;
            lastTab = tabIndex;
        }

        QWidget *host = page->parentWidget();
        QWidget *control = nullptr;
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        const QVariant current = CommanderSettings::value(setting.key, setting.defaultValue);
        const QString key = setting.key;
        switch (setting.kind) {
        case Kind::Bool: {
            auto *box = new QCheckBox(setting.label, host);
            box->setChecked(current.toBool());
            connect(box, &QCheckBox::toggled, this, [key](bool on) {
                CommanderSettings::setValue(key, on);
            });
            row->addWidget(box, 1);
            control = box;
            break;
        }
        case Kind::Int: {
            auto *label = new QLabel(setting.label, host);
            auto *spin = new QSpinBox(host);
            spin->setRange(setting.minimum, setting.maximum);
            spin->setSuffix(setting.suffix);
            spin->setValue(current.toInt());
            connect(spin, &QSpinBox::valueChanged, this, [key](int value) {
                CommanderSettings::setValue(key, value);
            });
            row->addWidget(label, 1);
            row->addWidget(spin);
            control = spin;
            break;
        }
        case Kind::Choice: {
            auto *label = new QLabel(setting.label, host);
            auto *combo = new QComboBox(host);
            for (const Choice &choice : setting.choices) {
                combo->addItem(choice.label, choice.value);
            }
            const int index = combo->findData(current.toString());
            combo->setCurrentIndex(index < 0 ? 0 : index);
            connect(combo, &QComboBox::activated, this, [key, combo]() {
                CommanderSettings::setValue(key, combo->currentData().toString());
            });
            row->addWidget(label, 1);
            row->addWidget(combo);
            control = combo;
            break;
        }
        case Kind::Text: {
            auto *label = new QLabel(setting.label, host);
            auto *edit = new QLineEdit(current.toString(), host);
            edit->setMinimumWidth(240);
            connect(edit, &QLineEdit::editingFinished, this, [key, edit]() {
                CommanderSettings::setValue(key, edit->text());
            });
            row->addWidget(label, 1);
            row->addWidget(edit);
            control = edit;
            break;
        }
        case Kind::Button: {
            auto *push = new QPushButton(setting.label, host);
            if (key == QLatin1String("ClearThumbnailCache")) {
                connect(push, &QPushButton::clicked, this, &CommanderSettingsDialog::clearThumbnailCache);
            } else if (key == QLatin1String("RestoreDefault")) {
                connect(push, &QPushButton::clicked, this, &CommanderSettingsDialog::restoreDefaults);
            } else {
                // Everything else is done by the main window, which listens for the button's key.
                connect(push, &QPushButton::clicked, this, [key]() {
                    CommanderSettings::notifier()->changed(QStringLiteral("button:") + key);
                });
            }
            row->addWidget(push);
            row->addStretch(1);
            control = push;
            break;
        }
        }
        control->setObjectName(QStringLiteral("setting-") + key);
        if (setting.support == Support::WindowsOnly) {
            control->setEnabled(false);
            control->setToolTip(windowsOnlyNote());
        } else if (setting.support == Support::NotYet) {
            control->setToolTip(notYetNote());
        }
        page->addLayout(row);
        if (!setting.info.isEmpty()) {
            auto *info = new QLabel(setting.info, host);
            info->setWordWrap(true);
            info->setEnabled(false);
            info->setContentsMargins(24, 0, 0, 0);
            page->addWidget(info);
        }
    }
    for (QVBoxLayout *page : std::as_const(pages)) {
        page->addStretch(1);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(buttons);
}

void CommanderSettingsDialog::restoreDefaults()
{
    for (const Setting &setting : settings()) {
        if (setting.kind == Kind::Button || setting.support == Support::WindowsOnly) {
            continue;
        }
        CommanderSettings::remove(setting.key);
        // The window shows the default again.
        QWidget *control = findChild<QWidget *>(QStringLiteral("setting-") + setting.key);
        if (auto *box = qobject_cast<QCheckBox *>(control)) {
            box->setChecked(setting.defaultValue.toBool());
        } else if (auto *spin = qobject_cast<QSpinBox *>(control)) {
            spin->setValue(setting.defaultValue.toInt());
        } else if (auto *combo = qobject_cast<QComboBox *>(control)) {
            const int index = combo->findData(setting.defaultValue.toString());
            combo->setCurrentIndex(index < 0 ? 0 : index);
        } else if (auto *edit = qobject_cast<QLineEdit *>(control)) {
            edit->setText(setting.defaultValue.toString());
        }
    }
}

void CommanderSettingsDialog::clearThumbnailCache()
{
    const QString cache = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/thumbnails");
    QDir(cache).removeRecursively();
}
