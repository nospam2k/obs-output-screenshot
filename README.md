# obs-output-screenshot

A minimal OBS plugin that captures the **live program output** and sends it to
connected obs-websocket clients as a base64 PNG — no AdvancedSceneSwitcher
required.

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

1. Download the zip for your platform from the Releases page.
2. Extract into your OBS plugins folder.
3. Restart OBS.
4. The plugin activates automatically — no configuration needed.

## Usage with obs-web

Use the included `index.html` as your obs-web frontend. It already uses the
`OutputScreenShot` vendor name and `getOutputScreenShot` message.
