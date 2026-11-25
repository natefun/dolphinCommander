# OneCommander parity: where dolphinCommander stands

OneCommander 3.108 runs under Wine 11.22 (prefix `~/.local/share/wineprefixes/onecommander`, launcher `~/.local/bin/onecommander`). It needed winetricks
dotnet48, d3dcompiler_47, corefonts and win10, WPF software rendering (`HKCU\Software\Microsoft\Avalon.Graphics` DisableHWAcceleration=1) and Wine's X11
driver, since the Wayland driver draws its windows blank. The old crash (0xe0434352 while loading the ModernWpf theme) was a TypeLoadException for the
WinRT type `Windows.UI.ViewManagement.UISettings`: ModernWpf.dll is patched with Mono.Cecil so the 8 ColorsHelper/UISettingsResources methods that
use it do nothing (original kept as `ModernWpf.dll.orig`). It still shows a cloud-folder warning at start (a false alarm: Wine has no NTFS transactions),
and its icon glyphs need Segoe MDL2 Assets (plus Segoe UI for text), which Wine lacks: they come from Microsoft's design downloads
(aka.ms/SegoeFonts, aka.ms/SegoeFluentIcons), copied into the prefix's `windows/Fonts` and registered under `HKLM\...\CurrentVersion\Fonts`.
Compiled from OneCommander's own help (onecommander.com/help2: Folder Columns and Navigation Pane,
Folder Pane, File Views, Browsers and Layouts, Sidebar, Details Pane, Main Window, Settings, Search,
Favorites, File Automator). Its Shortcut keys page (Shortcutkeys.html) has been read.
Nothing here has been checked by hand in the running app; the tests are in
`src/tests/dolphincommanderfeaturestest.cpp`.

## Have (before this branch)
Miller columns with preview, two sides with their own tabs, workspaces, folder hotlist, compare
folders, synchronized browsing, rename by pattern, checksums, quick view, archives through Ark,
type-to-filter, move-by-default drag, F5/F6 copy and move to the other side, back/forward,
hidden files, folders first, search (Dolphin's).

## Added on this branch
Select/deselect by pattern (Alt++ and Alt+-), open folder on the other side, swap sides, update the other side,
duplicate finder, recent folders in the hotlist, color labels (menu + row tint, stored as tags),
New Note (Alt+N), To-Do files with a checkbox in the row, Scripts menu, Folder Sizes dialog, Convert Images, Alt+C / Alt+M / Alt+S,
Paste Clipboard as File (Ctrl+Alt+V), Open With on Ctrl+Enter, middle-button selection rectangle, folder background image,
zoom and pan in the image preview, To-Do files (.2do/.2dx), Recent Notifications, Color Rows by
Age, Queue Copy and Move Jobs, Always Sort Folders by Name, Scroll by Whole Items, Color Theme (Follow the Desktop, Light, Dark),
Notifications panel (Ctrl+Shift+~), Toggle Miller Columns (Ctrl+`), Run with Arguments, New Tab from Clipboard Path,
Alt+middle click to send a tab to the other side, Layout menu (Standard/Columns/Classic), F7 to show only the names, Ctrl+Shift+F1 to center a lost window, Ctrl+Shift+F5 to reload the theme, File Views menu with saved views and the File View Designer, Up/Down in the quick view to step through the files.

Details Pane under each list (View > Details Pane): a thumbnail of the selected file and its name, kind, created, modified, age ("7 d", "9 M", "2 y") and size, and the counts of items and selected items; with no selection it describes the folder.
Colored Badges for Dates and Sizes (View > Appearance and Behavior, on by default): dates are drawn on a badge by age, sizes on one from green to red.
Nord color theme (Color Theme > Nord; the palette of OneCommander's Nord theme) and Compact Rows (View > Appearance and Behavior: sets the zoom of every view to its smallest).
Favorites panel with groups (Ctrl+Alt+F; View > Show Panels): folders kept in named groups, added with Ctrl+Alt+D, by dropping a folder on the panel, or from the panel's menu; groups can be made, renamed and removed. Kept in the settings.
Drive usage: pointing at a drive in the Places panel shows how full it is ("79% used, 205 GiB free of 959 GiB"). Not drawn on the entry itself, because the entries are painted by KIO's KFilePlacesView.
Main toolbar (bottom left of the sidebar, as OneCommander's): Change Layout (cycles Standard, Columns, Classic), File Automator (the Scripts menu), New Window and About.
OneCommander has no menu bar; its commands are in this toolbar, its context menus and its shortcut keys, so the fork keeps the menu bar hidden by default and the menus are grouped for those who show it (Ctrl+M).
User icons: a picture in ~/.config/dolphincommander/icons/ named after a suffix (txt.png) replaces the icon of those files, one named after a folder (Music.png, any case) replaces the icon of folders of that name, and folder.png the icon of all other folders (.png, .svg and .ico are read; the folder is read again when it changes).
Rich file view (File Views > Rich): tall rows with previews, so a text file shows its first lines and a picture its thumbnail.
New File from Template (Ctrl+N; New Window moved to Ctrl+Alt+Shift+N): a window lists the files of ~/Templates, takes a name and makes a copy, with Create, Create and Open and Edit Templates; File > Save as File Template copies the selected file there.
Copy Path of This Folder As (Edit menu; Ctrl+Alt+Shift+C copies it at once): plain, in quotes, escaped for the shell, or as a file:// address (OneCommander's backslash variants are Windows ones).
Copy Path of the Selected File (Ctrl+Shift+C; one path per line for several files). Paste Clipboard as File (Ctrl+Alt+V) asks what to do with a web address: a link file (.desktop), a text file or a download into the folder.
Tab menu: Copy Tab to the Other Side and Close Stale Tabs (those not shown for a day, counted from when they were made or last shown in this run).
Favorites panel: a History group (the last 20 folders opened in any tab, each once, newest first; Clear History in its menu), aliases for favorites (Change Alias…), and dragging favorites up and down or into another group.
Dock the window (global key Meta+Alt+\`, set through KGlobalAccel): the window comes to the lower half of its screen, as wide as the screen; once more, it is minimized. Thin Scrollbars (View > Appearance and Behavior) are style sheet rules added after the user's own sheet.
Thumbnails for Folders of Pictures (View > Appearance and Behavior, off by default): a folder of mostly pictures (at least three items, 60 percent images) with no style of its own is shown with larger rows and previews when it is read. Make This the Default File Manager sets the handler of inode/directory with xdg-mime.
The details pane has OneCommander's buttons beside its counts: cut, copy, rename and move to trash, which trigger the actions of the window.
Named windows: a workspace (Sides > Workspaces) now also keeps the layout and the position and size of the window, so opening it brings those back as OneCommander's named windows do.
F2 on several selected files opens the batch rename window (OneCommander opens its File Automator). Paste as Symbolic Links (Ctrl+Shift+V; OneCommander pastes shortcuts). Scripts get the environment variables DOLPHINCOMMANDER_SCRIPTS_DIR and DOLPHINCOMMANDER_SETTINGS_DIR (and OC_SCRIPTS_DIR and OC_SETTINGS_DIR, the names OneCommander uses). The details pane shows what the file says about itself: size of a picture, artist of a song, pages of a PDF.
Alt+P shows or hides the details pane, Ctrl+Alt+Shift+W closes all tabs but the current one, the color labels have the numbers 1 to 6 as accelerators in their menu, and a path on the clipboard that leads to a file opens its folder in a new tab and selects the file (Ctrl+Alt+Shift+T).
Files dropped on a favorite of the Favorites panel are moved into that folder (copied while Ctrl is held); they do not make a favorite. A folder dropped on a group's title or on the panel does.
Ctrl+V with text or a picture on the clipboard makes a dated file of it (with files on it, it pastes them as before). Ctrl+Shift+C with nothing selected copies the folder's path. New Folder… (Ctrl+Shift+N): a name, Enter makes the folder, Ctrl+Enter makes it and opens it, Shift+Enter makes it and moves the selected files into it. OneCommander's F10 for the terminal is not used: F10 is the menu key of KDE; Shift+F4 opens the terminal.
OneCommander's -b1 and -b2 (open a path in the first or second browser) are covered by `dolphincommander --split <path1> <path2>`: with two folders and --split, the first goes to the left side and the second to the right side, in a window that is already running too.
Variable scroll speed while dragging: Dolphin's list view already does it (the nearer the cursor is to the edge, the faster it scrolls, between a minimum and a maximum speed, building up gradually; KItemListView::calculateAutoScrollingIncrement).

## Dolphin already does (found while checking the list)
Tab aliases (Rename Tab), new tab after the last tab or the current one, remembering tabs and a start
folder, search from here or everywhere (Baloo), tags in the Places panel, hidden files, folders first,
Ctrl+click in a parent column of the Miller columns (it adds the item to that column's selection and closes the columns after it;
covered by `testSeveralSelectedItemsShowNoFolder` in `dolphincolumnsviewtest`), custom folder icons (the folder's Properties, which writes its .directory file), a delete key for the trash and permanent delete, Ctrl+Space to toggle the selection, "Open in Split View" on a place
(OneCommander's [1]/[2] buttons of a favorite), and backup files (~, .bak) hidden unless hidden files are shown
(OneCommander's "hide temporary files").

## Not done
- File View Designer: a window (File Views > Design This View) picks the columns, their order and widths and can keep the design under a name; "Hide Columns When Narrow" hides the columns that do not fit; each column can have its own width of the view under which it is hidden, set in the designer
- XAML themes: not as such. Themes are colors, a text size, an icon theme and a Qt style sheet (the program's own CSS-like language, edited in
  Color Theme > Style Sheet…), and they can be saved to a .theme file and loaded again; there is no XAML and no layout templates
- Copy and move queue: copy, move and paste from a view, a drop (drag and drop) and Update the Other Side are queued (the drop is not covered by a test; it needs a drag)
- Column hover-expand tuning: does not apply here. The Miller columns of the fork scroll sideways instead of shrinking
  the columns farther back in the path, so there is no compressed column to expand
- OneCommander's plain ~ for the notifications (Ctrl+Shift+~ opens the panel, Ctrl+Alt+Shift+~ the menu); Ctrl+` toggles the Miller columns

## From the later help pages (File View Designer, Resizing panes, Network drives, Phone storage, Preview, first start)
- Removal Order and Minwidth of a column: the designer's "Hide below" width per column does the same job, in pixels of the view instead of an order.
- Designs as .json files: File Views > Save View Design to File… and Apply View Design from File… (mode, previews, columns, widths).
- "Apply to the current folder and close, without saving": the designer has an "Apply Without Saving" button; the folder's own settings stay as they were.
- Preview Window: Space opens it in OneCommander, here it is Alt+F3 (Space selects); Up/Down step through the files and Esc closes it, as there.
  Word (.docx), Excel (.xlsx) and OpenDocument (.odt, .ods, .odp) files show their text in the preview (it needs KArchive, which is now a dependency).
  Quick view plays video and sound: Space plays and pauses, Left and Right seek by five seconds (Shift: an eighth of the length, so that seven presses review the whole video), [ and ] step by
  about one frame. A photo with a position in its metadata has a Show on Map button (OpenStreetMap). A picture can be shown Fit, Fill Width or at Actual Size
  (OneCommander has three viewing modes in its preview window, shown as buttons whose names its pages do not give, so these three are my reading of them). Like OneCommander, the quick view guesses the mode that suits a picture (its own size if it fits, as wide as the window for a very wide or tall one, else all of it) and its buttons show the guess. A checkbox makes videos and sounds play at once.
- Collapsing a side by dragging its divider: the fork's two sides do it (Ctrl+F3 and the splitter), the same as OneCommander's single-browser layout.
- Network drives and phones: OneCommander scans the Windows network and points to Phone Link; on Linux this is KIO (smb, sftp, ftp, mtp) in
  the Places panel and the location bar, which the fork inherits from Dolphin.
- First start asks for a layout and a theme (once; not in test mode). Keep Running When the Window Is Closed hides the window into the system tray (Show and Quit in its menu); a global key (Meta+Alt+E, set through KGlobalAccel, so the desktop can change it) brings the window back from the tray or from being minimized.

## Looked at, not only tested
`COMMANDER_SHOTS=<folder> build/bin/dolphincommanderfeaturestest testTakeScreenshots` saves screenshots of the main window (light and dark, the
Columns and Classic layouts) and of the tool windows into the folder.
Looking at them found: the Dark theme left the file lists light (they read the desktop's color scheme; fixed, they now follow the program's palette
while one of the fork's themes is on) and the quick view's picture used half the window (fixed).
Breeze icons are single-color and take their color from the desktop's color scheme, so a palette alone left them dark on dark. The Light and Dark themes
now activate the desktop's BreezeLight and BreezeDark color schemes when they are installed (the way View > Color Scheme does), and use the fork's own
palette only when they are not, or for Custom Colors. In the offscreen window used for the screenshots the icons stay dark even then, so whether the
schemes recolor them in a real desktop session is not verified.

## Keys of OneCommander's list that were left out
- `` ` `` alone, `~`, `\` alone, and the letters: type-to-filter needs them, so the fork uses Ctrl+` (columns), Ctrl+Shift+~ (notifications) and Alt+D / Ctrl+L (path).
- Shift+F1 (center the window): taken by What's This, so it is Ctrl+Shift+F1.
- Ctrl+I (properties): taken by the filter bar; Alt+Enter is the properties key.
- Ctrl+Shift+Enter (run with arguments), Ctrl+Enter (open with), F8 (lock the dividers of the Miller columns), Ctrl+F10 (terminal as administrator) and Ctrl+Shift+F12 are there. The last two go through pkexec (konsole for the terminal), were not tried with a real password prompt, and Ctrl+Shift+F12 opens a root copy of the window and leaves this one open.

## How this was tested
`build/bin/dolphincommanderfeaturestest` (84 tests, 1 skipped unless asked for screenshots). Each run starts from clean settings. A test of the whole suite with
`fork/check-tests.py` shows one test of Dolphin's columns view (`testTheContentWidthFollowsInsertedAndRemovedNames`) that fails under load and passes alone.

## Seen in the real OneCommander 3.108 (Wine, 2026-11-24)

First side-by-side look. Things the real window has that dolphincommander does not match yet:

- No menu bar at all (dolphincommander now has the menu button in each pane header, showing every menu; the bar itself is still on unless hidden with Ctrl+M). Each pane has a hamburger button (with a green dot) at the left of its path bar; the menus live there. (I could not open it by synthetic click under Wine, so its entries are still from the help pages.)
- (Drives, favorite group and History cards done: rounded cards above and below Places, each group with a "+" that adds the current folder; and a Network card whose title opens the network; Places still lists Dolphin's own Remote group.) Left column is a stack of rounded cards: Drives (letter, usage bar, "used / total GB"), Network, user groups ("..." and "+" buttons, then a "New group" button), and History. No Dolphin-style Places list.
- Window title strip shows the app name and a small button; tabs sit above each pane, each pane with its own "+" button.
- Pane header: path as `This PC \C: \users \nate` with backslashes, then the folder name in large grey type, "12 items" under it, then a row: back, forward, up, a folder button, history up/down chevrons, and four view-mode buttons (details, details with preview, list, tiles).
- Top right of each pane: sort, columns, "+", clipboard and lightning buttons, and the selection size ("0 B").
- Rows are compact, with a [DIR] marker, an age badge (colored by age: 15', 3d, 15d, 1y) and the date, and a checkbox at the right edge.
- Bottom-left toolbar: Settings, About, Layout (green dot), Search, File Automator, New Window. Item count at the bottom of each pane.
- Default look is a dark grey theme with blue folder icons.

Whole-screen captures show other windows; only the OneCommander window was captured, with `import -window <id>`.

## OneCommander's own menu captions (read from the strings in its installed program)

Its English captions are in its resources, so the menu wording does not have to be guessed. Context menu entries it has, and what dolphincommander does with each:

- Done now: Browse Phone and Scan Network (Go menu: opens mtp:/ and remote:/), Copy/Move to Recent Destination (File menu, remembers the last 10 copy and move folders), Hide drives from sidebar (View, Appearance and Behavior, or right-click the Drives card), Paste as Hard Links, Open shortcut destination (Open Link Destination), Run as different user, Regenerate thumbnail(s).
- Already there: New tab / New window / New tab in other pane, Close other tabs, Close stale tabs, Reopen closed tab (Dolphin's recently closed tabs), Copy as path, Copy/Move to the opposite pane, New folder from selection, New note, create file template, Rename with File Automator, Tag (color), sort orders, Change alias, favorite group move/rename/delete, scripts open/remove, Open in command shell.
- Not done: Map network drive and Mount folder as drive letter (Windows drive letters), Scan network (deep), Start/Stop monitoring a drive for changes.
- Preview (from its settings strings): small pictures are shown "Scaled to fit preview pane" or at "Original size"; "Image scaling interpolation" is Pixelated or Smooth (Quick view now has a Pixelated scaling checkbox); preview has "Multimedia" and "Text/plain" tabs; Esc or Space closes the preview; there is an option for QuickLook instead of the built-in preview (Windows only).

## OneCommander's Settings captions, checked against dolphincommander

Read from the 437 settings strings in its installed program. Behaviours that Dolphin or the fork already has: middle-click on a folder opens a tab, double-click on empty space goes up, folder background image (folder.jpg, cover.jpg), paste text and pictures as files, paste a web address as a link or download, thin scrollbars, folders sorted by name, auto-switch to thumbnails, hide/show hidden files, copy path format, restore tabs, tab history, drag and drop consistent move/copy, workspaces, custom icons, scripts.

Not done (and mostly Windows only): rename suggestions popup, mark recent files, Windows shell context menu tweaks, Explorer preview, QuickLook, acrylic and Mica window effects, hotkey Win+E, OneDrive status icons, "Open in OneCommander" registry entries, TeraCopy as the file operations handler, the Connector (dialog connector) for Open/Save dialogs, Unity thumbnail handler, and the yt-dlp video and audio download by pasting a URL.

## The lower-left row of buttons

One row, as in OneCommander's own window and with its tooltips (from its strings): Program settings, About/Help, Switch between Standard and Columns layouts (click cycles the layouts), Search files starting from currently opened folder, File Automator (the batch rename), Open new or saved window (a menu: Create new window, Saved windows = the workspaces, Center current window). The old Settings text button and the second row of buttons are gone. Not copied: the green dot on the Layout button, and a Scripts button (OneCommander shows it only when asked to; the scripts are in the Tools menu).

## The settings window

`Settings…` (Ctrl+comma, the gear in the lower left) opens a window with OneCommander's tabs (General, View, Columns, Preview, Theme, Window, Advanced, Other) and its groups and wording, built from the strings in its program. Every change is applied and saved at once; the settings live in the `Commander` group of the configuration, so a menu entry and its setting in the window are the same thing. Dolphin's own settings stay in `Configure dolphinCommander…`.

Each setting is one of three kinds, and the table in `src/settings/commandersettingsdialog.cpp` says which: applied (the program does what it says), for Windows only (shown, cannot be changed: Mica, the registry, Explorer, OneDrive and so on), or kept but not applied yet (it is saved and shows a tooltip saying so). The counts are in the commit message of the latest settings commit.

## Settings that are Windows only (shown, cannot be changed)

- ShowSystem: Show protected system files and folders
- ShowFileAttributes: Show file attributes and synchronization state icons
- SkipSearchingReparsePoints: Skip searching within reparse points
- UseLocalThumbsDb: Use local thumbnails database (faster)
- UsePDFThumbs: Custom .pdf and .ai thumbnail extraction
- GetCloudThumbs: Get thumbnails of cloud files
- RegisterMaxThumbs: Register 3ds Max thumbnail handler
- RegistryOfficeThumbs: Force Microsoft 365/Office to save thumbnails in files
- DownloadIconPacks: Download icon packs
- PreviewWindow: Preview window
- PreviewDownloadsCloudFiles: Download cloud files when previewing
- EnableSFP: Enable Explorer Preview (Shell File Preview)
- UseSystemAccentColor: Use system accent color
- UseAcrylicWindow: Use Acrylic window
- UseMicaWindow: Use Mica window effect
- UseAcrylicMenu: Use acrylic effect in menu
- UseLayoutRounding: Pixel-perfect layout
- TextRendering: Text rendering
- Rendering: Rendering
- ShowEjectOnSubstDrives: Show Eject button on Subst drives (folders mounted as drives)
- ImportFavoritesFromV2: Import Favorites from V2
- ShowWindowNameInTaskbar: Show window name in Taskbar
- ShortcutClick: If program has a window open, double-clicking its shortcut should:
- ShowErrorReportDialog: Show error report dialog on crash
- UseOldDragAndDropSystem: Use old Drag&Drop system
- FixStylusLag: Fix Stylus lag (breaks touchscreen double-tap to open)
- FileOperationsHandler: File operations handler
- UseShellContextMenuSyncCascadeMenu: Wait for all items before showing context menu
- ShowOverlayOnShellContextMenu: Block window when Shell Context Menu is open
- GetRecentMenuItems: Get recent menu items
- OpenOfficeFilesThroughExplorer: Open Office files through Explorer
- ExcludedShellContextMenuItems: Excluded Shell Context Menu items
- WinERegister: Register Win+E hotkey
- DialogConnector: Dialog connector
- UpdateToVersionAhead: Update to version ahead (less stable)
- UseOneCommanderUpdater: Use external updater program instead of PowerShell script
- AddToEnvironmentPath: Add to user PATH variable
- CreatePSAlias: Create PowerShell alias 'oc'
- RegistryAddOpenInOC: Add \
- EmailNamingFormat: Email naming format (drag from Outlook)
- OneDriveFolderPath: Path to OneDrive folder

## Settings kept but not applied yet

- ShowRenameSuggestions: Show rename suggestions popup
- UseHighResScrolling: Use high-resolution touchpad scrolling
- RemoveDuplicatesFromTabsHistory: Remove duplicates from tab's path history
- UseSingleNewItemButton: Show a single New Item button instead of 3 buttons for New File/Folder/Note
- ShowKeepFoldersOnTopToggle: Show toggle for listing folders above files
- LiveSort: After renaming or updating file:
- ShowFolderSizeOnNetwork: Show Folder size on network folders
- ShowMetadataOnNetwork: Show Metadata on network files
- ExcludedWatcherKeywords: Excluded watcher keywords
- ItemBackgroundColorSource: Item coloring
- GridViewFileNamePosition: File name position in Grid view
- MetadataLineCount: Lines of text
- MetadataExcerpts: Show text excerpts for these file types
- FolderThumbnailStyle: Folder thumbnail style
- FileIconsPack: File icons pack
- FolderIconsPack: Folder icons pack
- ColumnWidthCurrentStandardLayout: Current (rightmost) column pane width in Standard layout
- ColumnAlignStandardLayout: Column Pane alignment in Standard Layout
- ColumnAlignColumnsLayout: Column Pane alignment in Columns Layout
- ColumnAutoExpandStandardLayout: Expand Column when cursor enters it (Standard layout)
- ColumnAutoExpandColumnsLayout: Expand Column when cursor enters it (Columns layout)
- ColumnAdvancedExpandTriggerWidth: Trigger width
- ColumnAdvancedDelay: Expand delay (ms)
- ColumnAdvancedDuration: Expand duration (ms)
- CreateNewColumnsWithoutAnimating: Create New Columns without animating
- ContextMenuItemPadding: Context Menu Item padding
- FocusedElementStyle: Keyboard-focused element display style
- HotkeyVizor: Use global hotkey Ctrl+` to show docked program window
- ExtensionsExecutable: File types (executable)
