# Deployment onboarding UI

The file setup flow a game shows on a device that has no game files yet, and the
two surfaces it appears on.

## Language

**Onboarding view**:
The file setup flow a player sees on a device that has no game files yet.
_Avoid_: setup screen, first-run wizard, installer

**Console surface**:
The onboarding view as drawn on the game screen, driven by a controller.
_Avoid_: setup window, native UI, SDL UI

**Phone surface**:
The onboarding view served over HTTP to a browser on the same network, used to
send files without a keyboard or a file picker.
_Avoid_: upload page, web UI, webview

**Flavor**:
Build-time selection of the toolkit that renders the console surface, either
imgui or sdl3. A flavor is not a product variant: both render the same design.
_Avoid_: skin, theme, variant, backend, mode

**Flat-neutral**:
The onboarding view's visual language: neutral dark greys, one accent, no
gradients, no brand colour. Deliberately unlike a launcher's palette.
_Avoid_: dark theme, minimal theme, system style

**Scene**:
The rectangles, text runs, bars, code and buttons a page consists of, before
anything paints them.
_Avoid_: layout tree, draw list, view model
