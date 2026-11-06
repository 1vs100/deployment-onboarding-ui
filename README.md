# Deployment onboarding UI

The file setup screen a game shows on a device that has no game files yet, plus the phone page that sends them over the local network.

- One design, two renderers: pick Dear ImGui or plain SDL3 at build time.
- Every file is listed with its hash and size, so nothing unverified reaches the game.
- A browser on the same network can upload; the console shows a QR code to reach it.
- Controller, keyboard and mouse all drive the same focus ring.
- Built to be consumed as a git submodule: you supply the pickers, the sockets and the user folder.

Screenshots of every page in both flavors are in `docs/screenshots`.

## Using it

The library exports one entry point and one struct of platform functions:

```cpp
#include <onboarding/gate.h>

onboarding::Platform platform;
platform.run_worker = [](std::function<void()> work) { /* run off the UI thread */ };
platform.pick_folder = [](onboarding::FileStore& store, onboarding::Catalog& catalog) {
  // open a folder picker, then copy on a worker
};
platform.pick_files = [](onboarding::FileStore& store, onboarding::Catalog& catalog) {
  // open a file picker, then copy on a worker
};
platform.pickers_available = [] { return true; };
platform.transfer_active = [] { return false; };
platform.lan_urls = [] { return std::vector<std::string>{"http://192.168.1.42"}; };
platform.make_upload_server = [](onboarding::FileStore&, onboarding::Catalog&) { return nullptr; };
platform.log = [](const std::string& line) { /* your log */ };

const bool ready = onboarding::EnsureGameFiles(user_folder / "gamedata",
                                               {"1 vs 100", "2.0.1149.0", "0.1.151.3"},
                                               std::move(platform));
```

`EnsureGameFiles` checks the files first. When they are all in place it returns
true without opening a window. Otherwise it runs the flow on the calling thread
and returns true only if the player pressed Continue.

The embedded file list comes from `ONBOARDING_UI_MANIFEST`, a text file of
`<sha256> <size> <kind> <relative path>` lines, with kinds `game`, `launch`,
`show`, `episode` and `avatar`.

Serving the phone page needs two more calls, because sockets belong to the host:

```cpp
#include <onboarding/web.h>

server->on_page = [] { return onboarding::WebPage(); };          // GET /
server->on_status = [&] { return onboarding::StatusJson(catalog.Take()); };  // GET /status
file_store.Ingest(index, read);                                   // PUT /upload/<path>
```

The page, the JSON and the ingest rules live in this library; the host owns the
listener because only it knows whether the platform has sockets at all.

## Flavors

`-DONBOARDING_UI_FLAVOR=sdl3` (default) paints the scene with SDL renderer
primitives and a glyph atlas built from the embedded font. It links nothing
beyond SDL.

`-DONBOARDING_UI_FLAVOR=imgui` paints the same scene with ImGui draw lists,
using the vendored ImGui 1.91.9b and its SDL3 and SDL renderer backends.

Both flavors show the same design; see `docs/design.md` for the rule that keeps
them together, and `docs/adr/0001-flavor-is-a-toolkit-choice.md` for why both
exist.

## Regenerating the screenshots

```
cmake -S . -B build/sdl3 -DONBOARDING_UI_FLAVOR=sdl3
cmake --build build/sdl3 -j8
SDL_VIDEODRIVER=dummy ./build/sdl3/deployment_onboarding_demo --capture docs/screenshots
```

The demo renders the real pages against a fixture manifest, so it needs no game
files, no device and no compositor: the dummy video driver plus the software
renderer write the PNGs directly. Repeat with `-DONBOARDING_UI_FLAVOR=imgui` into
`build/imgui` for the other flavor.

The phone page is captured with headless Firefox, which takes the shot at the
load event, so the fixture status is injected into a copy of the page:

```
python3 - > /tmp/phone_capture.html <<'EOF'
page = open('src/web/index.html').read()
state = open('/tmp/status.json').read()
print(page.replace('</body>', f"<script>status={state};render();</script></body>"))
EOF
```

`/tmp/status.json` is the fixture in the shape `StatusJson` produces. Serve that
file at `/status` and the copy at `/`, then:

```
MOZ_ENABLE_WAYLAND=0 firefox --headless --no-remote --new-instance \
  --profile /tmp/ffprof --window-size=760,1320 \
  --screenshot="$PWD/docs/screenshots/phone.png" http://127.0.0.1:8792/
```

## Layout

```
include/onboarding/   the API a host uses: gate.h, platform.h, files.h, web.h
src/core/             manifest, per-file state, hash store, and the flow
src/platform/         file access and hashing per platform
src/ui/               design.h, scene.h, layout.cpp, input, view, one file per flavor
src/web/              the phone page and its JSON
demo/                 the capture harness and the fixture manifest
tests/                layout invariants
third_party/          ImGui and the embedded font
```

## Building

Needs C++23 and CMake 3.25. SDL3 comes from the host, or from `find_package` on a
desktop.

```
cmake -S . -B build -DONBOARDING_UI_FLAVOR=sdl3
cmake --build build -j8
ctest --test-dir build
```

## License

MIT. Vendored components are listed in `THIRD_PARTY_NOTICES.md`; the manifest
format and the upload protocol come from
[tuxuser/sdl-uwp-deployment](https://github.com/tuxuser/sdl-uwp-deployment).
