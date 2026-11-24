# OneCommander parity: where dolphinCommander stands

Wine was tried for a side-by-side run (Wine 11.22, .NET 4.8 installed by the user with winetricks): OneCommander 3.108 starts, shows its cloud-folder
warning (a false alarm: Wine has no NTFS transactions) and then dies with an unhandled .NET exception (0xe0434352) while it loads its ModernWpf theme.
It reports "Windows 7 or 8 detected" when the prefix says Windows 7 and crashes at the same point on Windows 10, so no window of it was seen. The look below comes from OneCommander's own screenshots and help instead.
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
- OneCommander's plain ~ for the notifications (Ctrl+Shift+~ opens the panel, Ctrl+Alt+N the menu); Ctrl+` toggles the Miller columns

## From the later help pages (File View Designer, Resizing panes, Network drives, Phone storage, Preview, first start)
- Removal Order and Minwidth of a column: the designer's "Hide below" width per column does the same job, in pixels of the view instead of an order.
- Designs as .json files: File Views > Save View Design to File… and Apply View Design from File… (mode, previews, columns, widths).
- "Apply to the current folder and close, without saving": the designer has an "Apply Without Saving" button; the folder's own settings stay as they were.
- Preview Window: Space opens it in OneCommander, here it is Alt+F3 (Space selects); Up/Down step through the files and Esc closes it, as there.
  Word (.docx), Excel (.xlsx) and OpenDocument (.odt, .ods, .odp) files show their text in the preview (it needs KArchive, which is now a dependency).
  Quick view plays video and sound: Space plays and pauses, Left and Right seek by five seconds (Shift: a tenth of the length), [ and ] step by
  about one frame. A photo with a position in its metadata has a Show on Map button (OpenStreetMap). A picture can be shown Fit, Fill Width or at Actual Size
  (OneCommander has three viewing modes in its preview window; its page does not say which, so these three are my reading of it).
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
`build/bin/dolphincommanderfeaturestest` (77 tests, 1 skipped unless asked for screenshots). Each run starts from clean settings. A test of the whole suite with
`fork/check-tests.py` shows one test of Dolphin's columns view (`testTheContentWidthFollowsInsertedAndRemovedNames`) that fails under load and passes alone.
