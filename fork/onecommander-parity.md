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
New Note (Alt+N), Scripts menu, Folder Sizes dialog, Convert Images, Alt+C / Alt+M / Alt+S,
Paste Clipboard as File (Ctrl+Alt+V), Open With on Ctrl+Enter, middle-button selection rectangle, folder background image,
zoom and pan in the image preview, To-Do files (.2do/.2dx), Recent Notifications, Color Rows by
Age, Layout menu (Standard/Columns/Classic), File Views menu with saved views.

## Dolphin already does (found while checking the list)
Tab aliases (Rename Tab), new tab after the last tab or the current one, remembering tabs and a start
folder, search from here or everywhere (Baloo), tags in the Places panel, hidden files, folders first,
a delete key for the trash and permanent delete, Ctrl+Space to toggle the selection, "Open in Split View" on a place
(OneCommander's [1]/[2] buttons of a favorite), and backup files (~, .bak) hidden unless hidden files are shown
(OneCommander's "hide temporary files").

## Not done
- File View Designer (column order, widths, which columns hide when narrow)
- XAML themes, custom folder icons
- Copy and move queue
- Ctrl+click in a parent column to add to the selection
- Settings: sort folders by name always, pixel/item scrolling, column hover-expand tuning
- Notes and To-Dos shown in the name column, with a checkbox
- Shift+` notifications pane, backtick toggle between breadcrumb and columns
