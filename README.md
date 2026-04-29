# obs-output-screenshot

A minimal OBS plugin that captures the **live program output** and sends it to
connected obs-websocket clients as a base64 PNG.

## How it works

1. The browser calls `CallVendorRequest` with vendor `OutputScreenShot` and
   message `getOutputScreenShot` (once per second).
2. The plugin captures OBS's composited program output via the graphics API.
3. It encodes the frame as a PNG and fires a `VendorEvent` back to all
   websocket clients.
4. The browser receives the event and updates the `<img>` element.

## Building

### Prerequisites

- CMake 3.22+
- OBS Studio 30+ with development headers
- obs-websocket (ships with OBS 28+)

### macOS

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

Copy `build/obs-output-screenshot.dylib` to:
```
~/Library/Application Support/obs-studio/plugins/obs-output-screenshot/bin/
```

### Windows

```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config RelWithDebInfo
```

Copy `build\RelWithDebInfo\obs-output-screenshot.dll` to:
```
%APPDATA%\obs-studio\plugins\obs-output-screenshot\bin\64bit\
```

### CI (both platforms via GitHub Actions)

Push a tag to trigger a release build:

```sh
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions will build macOS + Windows and attach zips to the release.

## Installation

### macOS

1. Download `obs-output-screenshot-macos.zip` from the Releases page.
2. Extract it — you'll get `obs-output-screenshot.plugin`.
3. Move the bundle to:
   ```
   ~/Library/Application Support/obs-studio/plugins/
   ```
4. Restart OBS.

### Windows

1. Download `obs-output-screenshot-windows.zip` from the Releases page.
2. Extract and copy `obs-plugins\64bit\obs-output-screenshot.dll` to:
   ```
   %APPDATA%\obs-studio\obs-plugins\64bit\
   ```
3. Restart OBS.

## Usage with obs-web

Use the included `www/index.html` as your obs-web frontend. It already uses the
`OutputScreenShot` vendor name and `getOutputScreenShot` message.

## OBS version compatibility

The plugin is built against a specific OBS version (see `OBS_VERSION` in
`.github/workflows/build.yml`). It should continue to work across minor OBS
updates, but a major OBS release may require a rebuild.

**If the plugin stops working after an OBS upgrade**, check the OBS log
(Help → Log Files → View Current Log) for lines tagged `[OutputScreenShot]`:

- `Loading plugin v… (built against OBS …)` — confirms the plugin loaded; the
  OBS version shown is what it was compiled against.
- `obs_websocket_register_vendor failed` — obs-websocket API changed or is not
  loaded; update the plugin.
- `Failed to register vendor request` — request type registration failed;
  update the plugin.
- `Captured frame is entirely black` — the render pipeline changed; update the
  plugin.
- `Failed to capture program output` — graphics API call failed; update the
  plugin.

To update for a new OBS version, bump `OBS_VERSION` in
`.github/workflows/build.yml`, push a new tag, and install the new release.
