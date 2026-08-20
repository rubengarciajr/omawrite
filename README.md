# Omawrite

A dead-simple Markdown writing app built with Qt Quick and C++ that automatically follows system dark/light mode.

Source and releases: [github.com/rubengarciajr/omawrite](https://github.com/rubengarciajr/omawrite)

<img width="2948" height="3227" alt="screenshot-2026-06-23_15-24-08" src="https://github.com/user-attachments/assets/4e930c0d-edda-4046-b444-a59eff523329" />
<img width="2948" height="3227" alt="screenshot-2026-06-23_15-23-23" src="https://github.com/user-attachments/assets/8ced7c26-961b-4ded-b263-84403001a951" />


## Install

On Omarchy, install the `omawrite` package from the Omarchy Package Repository. New Omarchy installs from Quattro forward include it by default.

To build this tree on Arch Linux or Omarchy:

```sh
./bin/test      # compile and run the suite
./bin/build     # compile the app into build/omawrite
./bin/package   # produce an Arch package under pkgbuild/
./bin/install   # build, package, and install with makepkg
```

`./bin/install` needs `makepkg` and sudo. After install, open a note with:

```sh
omawrite path/to/note.md
```

Drop a Markdown file onto an open window to open it. Multiple files on the command line open extra windows.

GitHub Releases attach the Arch package built from this repository.

## Shortcuts

- `Ctrl+S` saves. Unsaved documents use the XDG desktop portal file picker.
- `Ctrl+Shift+S` saves as.
- `Ctrl+O` opens a Markdown file through the portal picker.
- `Ctrl+P` opens the system print dialog.
- `Ctrl+N` opens a new Omawrite window.
- `Ctrl+Z`, `Ctrl+Shift+Z`, and `Ctrl+Y` handle undo and redo.
- `Super+F` toggles fullscreen. Qt maps this key as `Meta+F`.
- `Ctrl+F` searches the document. Use `Enter` or `Ctrl+G` for the next match and `Shift+Enter` for the previous match.
- `Ctrl+H` opens find and replace.
- `Ctrl+B`, `Ctrl+I`, and `Ctrl+K` insert bold, italic, and link Markdown.
- `Ctrl+,` opens the Type Foundation panel.
- `Ctrl+?` shows the keyboard shortcut reference.

## Omawrite Commands

Type `/` at the start of a line to open the command palette. Keep typing to
filter it, use the arrow keys to move, press `Enter` or `Tab` to insert, and
press `Escape` to close it.

The palette contains eight focused writing commands: Text, To-do List,
Heading 1, Heading 2, Heading 3, Bullet List, Numbered List, and Quote.

Image support is intentionally paused while it is rebuilt in smaller,
testable steps. See the [media upgrade plan](docs/media-upgrade-plan.md).

Use the `Aa` button in the footer to open the Type Foundation panel. Body,
Heading 1, Heading 2, and Heading 3 have independent persistent sizes that
apply live and continue to respect Omarchy's desktop text scale. List bullets,
numbers, and task markers use the active theme accent.

## Omarchy themes

Omawrite follows the active Omarchy theme live. Its editor palette comes from
`colors.toml`; menus, dialogs, control states, spacing, and rounding use the
matching semantic roles from `shell.toml` and Hyprland. Theme changes are
watched at runtime, so an open window updates without a restart. No interface
color is hardcoded in QML.

Unsaved drafts are recovered after an abnormal exit. Omawrite also watches open files
and warns before an external change can replace local work.

Text follows the desktop text size — `omarchy display text size`, or GNOME's
`text-scaling-factor` — and re-flows without a restart. The default of 12px leaves
Omawrite at the size it is designed around; larger and smaller sizes scale from there.

## Requirements

- Qt 6: `qt6-base`, `qt6-declarative`, `qt6-quickcontrols2`
- `xdg-desktop-portal` and a portal backend

The iA Writer Mono font is bundled under the SIL Open Font License 1.1; see
`fonts/OFL.txt`. The font is copyright Information Architects Inc. and based on
IBM Plex, copyright IBM Corp.
