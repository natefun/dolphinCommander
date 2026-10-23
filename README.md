# dolphinCommander

dolphinCommander is a fork of [Dolphin](https://apps.kde.org/dolphin/), KDE's file manager, that works more like [OneCommander](https://onecommander.com/): two panes side by side, each with its own tabs, a path bar and Miller columns, and a ribbon instead of menus.

It is an independent project and is not affiliated with or endorsed by KDE.

> **Status:** experimental and used by its author. It builds and runs, but it has not been tested widely and the unit tests have not been updated for the new window layout.

## What is different from Dolphin

- **Two panes, always.** The window is split into a left and a right side. Each side has its own tab bar and its own tabs, with a new-tab button that is always visible.
- **Hide either side.** Two arrows in the middle of the divider hide the left or the right side. The divider then stays at the window edge and the same arrows bring the side back. Dragging the divider to an edge does the same.
- **Copy and move between sides.** `F5` copies and `F6` moves the selection to the folder shown in the other side. `Ctrl+F3` moves the keyboard focus to the other side. "Open in Other Pane" is in the context menu.
- **Double-click to go up.** Double-clicking empty space in a folder goes up one folder. You can change that in Settings under View, General.
- **Path bar above each side.** Every side has a large breadcrumb path bar that always shows the full path.
- **Miller columns.** Above each file list is a strip with one column for each parent folder of the current folder. You can resize the columns by dragging the dividers. They scale with the window, and the columns farthest back are hidden when there is not enough room, so the strip never scrolls sideways.
- **A ribbon.** The hamburger menu and the toolbar are replaced by a ribbon with a tab for each of File, Edit, View, Tools and Settings. Back, forward, up and search are next to the tabs.
- **No telemetry and no links to KDE.** KUserFeedback, the KDE Store download button, and the About KDE, Report Bug and Donate entries are removed. There is no address that crash reports are sent to.
- **Removed from Dolphin:** the Folders panel, the toolbar, and the toggle for split view, since there are always two sides.

It can be installed next to Dolphin. It uses its own program name (`dolphincommander`), its own settings file (`~/.config/dolphincommanderrc`) and its own D-Bus name, and it does not take over the file manager D-Bus service.

## Building

dolphinCommander is based on the development version of Dolphin (KDE Gear 26.11), so it needs recent Qt 6 and KDE Frameworks 6 (6.23 or newer) with their development files.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
cmake --install build
```

Then start `dolphincommander`. To try it without installing, run `build/bin/dolphincommander` with `LD_LIBRARY_PATH=build/bin`.

CMake lists any missing packages when it configures. Baloo (file search and the information panel) and PackageKit are optional.

## Known limits

- Which side is hidden is not saved. Both sides are shown again after a restart.
- `F5` and `F6` also work while the other side is hidden, and then copy to a folder you cannot see.
- The unit tests in `src/tests` still assume the single tab widget of Dolphin and are not built by default.

## Where the new code is

- `src/dolphinmainwindow.cpp`: the two sides, which side is active, and the ribbon and divider setup
- `src/sidessplitter.*`: the divider between the sides, with its arrows
- `src/views/columnsstrip.*`: the Miller columns
- `src/ribbonbar.*`: the ribbon

## Upstream and license

dolphinCommander follows [KDE's Dolphin](https://invent.kde.org/system/dolphin), and most of the code is theirs. The Dolphin documentation at https://userbase.kde.org/Special:myLanguage/Dolphin still applies to the parts that are unchanged. Please do not report problems in dolphinCommander to KDE. Use this repository's issue tracker.

Dolphin and dolphinCommander are free software under the GNU General Public License, version 2 or later. See the files in `LICENSES/`.
