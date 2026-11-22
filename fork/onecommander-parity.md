# OneCommander parity: where dolphinCommander stands

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
Alt+middle click to send a tab to the other side, Layout menu (Standard/Columns/Classic), File Views menu with saved views and the File View Designer, Up/Down in the quick view to step through the files.

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
- Copy and move queue: copy, move and paste from a view, and Update the Other Side, are queued; a drop (drag and drop) is not
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
  about one frame. Not done: three viewing modes and a map button for photos with GPS.
- Collapsing a side by dragging its divider: the fork's two sides do it (Ctrl+F3 and the splitter), the same as OneCommander's single-browser layout.
- Network drives and phones: OneCommander scans the Windows network and points to Phone Link; on Linux this is KIO (smb, sftp, ftp, mtp) in
  the Places panel and the location bar, which the fork inherits from Dolphin.
- First start asks for a layout and a theme (once; not in test mode). Keep Running When the Window Is Closed hides the window into the system tray (Show and Quit in its menu); the global Win+Alt+E key to bring it back is not done.
