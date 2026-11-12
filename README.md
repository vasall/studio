# Vasall Studio

## Build Guide

To build Vasall Studio you first need to install the necessary dependencies:

### SDL3

To install SDL3 you have to get all necessary dependencies. The list can be
found [here](https://wiki.libsdl.org/SDL3/README-linux#build-dependencies).
Afterward you can follow this [guide](https://gist.github.com/NoxFly/1067c9fc24024d26b51a6825de5cff74) to install the SDL3 Core.

### Building and running

The Makefile builds the devkit (`../devkit`) first and links against it.

```sh
make          # build/vasall-studio and the tests
make run      # build and start
make test     # run the unit tests (layout, UI state, widgets, dock, core)
make clean
```

The same commands are available as VS Code tasks (`studio: build`,
`studio: run`, `studio: test`).

### Assets

- `assets/fonts/` - Inter and JetBrains Mono (SIL Open Font License, see the
  `OFL.txt` next to each font)
- `assets/icons/` - placeholder icons made by
  `python3 tools/gen_placeholder_icons.py`; replace them with your own, the
  file names are listed in `include/render/icon_ids.h`. To add new
  placeholders without overwriting replaced icons, name them:
  `python3 tools/gen_placeholder_icons.py assets/icons check folder`
- `assets/shaders/` - the UI shader
- `assets/theme/carbon-g100.theme` - colors and style classes of the UI,
  using the IBM Carbon Gray 100 color tokens (the default)
- `assets/theme/dark.theme` - the earlier VS Code-like dark theme; start
  with `VASALL_THEME=dark ./build/vasall-studio` to use it, or pass any
  other file name from `assets/theme/` without `.theme`. It has no styles
  for the newer widgets yet.

### Widgets

All widgets are in `include/ui/widgets.h`; *View > Open UI Gallery* (open
at startup) shows each of them:

label, icon, button, icon button, tooltip, tab, splitter, scroll area,
menu / context menu, checkbox, radio button, single-line text field,
slider, progress bar, dropdown, badge, tree row, virtualized list and
notifications (toasts).

Tab and Shift+Tab move the keyboard focus between focusable widgets and
scroll them into view; Esc or a click elsewhere drops it.

### Keyboard shortcuts

| Key             | Action                 |
|-----------------|------------------------|
| Ctrl+N          | New dummy tab          |
| Ctrl+W          | Close editor           |
| Ctrl+B          | Toggle side bar        |
| Ctrl+J          | Toggle panel           |
| Ctrl+\\          | Split editor right     |
| Ctrl+Shift+\\    | Split editor down      |
| F12             | Debug overlay          |
| Tab / Shift+Tab | Next / previous widget |

### Development aid

`VASALL_SCREENSHOT=out.ppm ./build/vasall-studio` draws a few frames, saves
the window as a PPM image and quits.
