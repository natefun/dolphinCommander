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
Alt+middle click to send a tab to the other side, Layout menu (Standard/Columns/Classic), File Views menu with saved views and the File View Designer.

## Dolphin already does (found while checking the list)
Tab aliases (Rename Tab), new tab after the last tab or the current one, remembering tabs and a start
folder, search from here or everywhere (Baloo), tags in the Places panel, hidden files, folders first,
custom folder icons (the folder's Properties, which writes its .directory file), a delete key for the trash and permanent delete, Ctrl+Space to toggle the selection, "Open in Split View" on a place
(OneCommander's [1]/[2] buttons of a favorite), and backup files (~, .bak) hidden unless hidden files are shown
(OneCommander's "hide temporary files").

## Not done
- File View Designer: a window (File Views > Design This View) picks the columns, their order and widths and can keep the design under a name; "Hide Columns When Narrow" hides the columns that do not fit; each column can have its own width of the view under which it is hidden, set in the designer
- XAML themes: there are three color themes (Follow the Desktop, Light, Dark) and one with four colors you choose; no styles, icons or sizes
- Copy and move queue: copy, move and paste from a view, and Update the Other Side, are queued; a drop (drag and drop) is not
- Ctrl+click in a parent column to add to the selection
- Settings: column hover-expand tuning
- OneCommander's plain ~ for the notifications (Ctrl+Shift+~ opens the panel, Ctrl+Alt+N the menu); Ctrl+` toggles the Miller columns
