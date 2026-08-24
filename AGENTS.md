# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Swiftray is a free, open-source C++17 desktop application for controlling grbl-based laser cutters and engravers. It uses Qt 6.7.2 with QML for the UI and targets macOS and Windows. Licensed under GPLv3.

## Build Commands

### macOS (CMake)
```bash
# First time: init submodules
git submodule update --init --recursive

# Build sentry-native (one-time, requires Qt path)
cd third_party/sentry-native
cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSENTRY_BACKEND=crashpad -DSENTRY_INTEGRATION_QT=YES -DCMAKE_PREFIX_PATH=<PATH_TO_QT>/lib/cmake
cmake --build build --config RelWithDebInfo --parallel
cmake --install build --prefix install
cd ../..

# Build application
mkdir build && cd build
cmake ..
make -j12

# Create distributable macOS app bundle
cmake --build . --target swiftray_app_bundle -j8
```

### Windows
Uses Visual Studio with vcpkg. The vcpkg toolchain is hardcoded in `CMakeLists.txt`. After building, run `windeployqt` to gather Qt runtime dependencies.

### Tests (Google Test)
```bash
cd build
make UnitTest
./tests/UnitTest
```

### Documentation
```bash
doxygen Doxygen
# View docs/index.html
```

## Key Build Configuration

- **C++ Standard:** C++17 (required)
- **Qt Keywords:** Disabled via `-DQT_NO_KEYWORDS` — use `Q_EMIT`, `Q_SIGNAL`, `Q_SLOT` instead of `emit`, `signal`, `slot`
- **Optimization:** `-O2`
- **macOS deployment target:** 10.15
- **Platform CMake files:** `cmake/Qt6/unix/CMakeLists.txt` (macOS), `cmake/Qt6/windows/CMakeLists.txt` (Windows)
- **Qt path** is hardcoded in the platform CMake file — update `QT_ROOT_DIR` for your local environment

## Architecture

### Entry Points (`src/main.cpp`)
- **GUI mode** (default): Initializes Sentry, loads QML engine, creates `MainWindow`
- **CLI mode:** `./Swiftray cli <file>` — loads SVG into virtual canvas
- **Daemon mode:** `./Swiftray --daemon` — runs headless with WebSocket server

### Core Source Modules (`src/`)

| Module | Purpose |
|---|---|
| `canvas/` | Main drawing surface, rendering, interactive controls (grid, rulers, selection) |
| `shape/` | Shape hierarchy: `Shape` base → `PathShape`, `BitmapShape`, `TextShape`, `GroupShape` |
| `parser/` | File format import: SVG, DXF, PDF (custom SVG handler) |
| `toolpath_exporter/` | G-code generation, machine-specific factories, trajectory generators |
| `executor/` | Job execution engine, machine commands, operation handling |
| `machine/` | Machine configuration and control |
| `periph/` | Serial port and motion controller peripherals |
| `connection/` | Serial communication layer |
| `server/` | WebSocket server for daemon mode |
| `windows/` | Qt windows, dialogs, and QML files (`windows/qml/`) |
| `widgets/` | Custom Qt widgets and panels (layers, tools, properties) |
| `settings/` | Machine and user settings persistence |
| `document.h/cpp` | Document model holding layers and shapes |
| `layer.h/cpp` | Layer abstraction with laser parameters |

### Key Classes
- `Canvas` — registered as QML type `Swiftray.Canvas`, main drawing/interaction surface
- `Document` — document model with layers and shapes
- `MainWindow` — primary application window
- `MainApplication` — QApplication subclass with global state
- `Executor` — job execution engine

### Third-party Libraries (`third_party/`)
- **sentry-native/crashpad** — crash reporting (must be pre-built)
- **libdxfrw** — DXF file reading
- **QxPotrace** — image tracing wrapper
- **liblcs** — color management
- **clipper** — geometry/polygon operations

### Major External Dependencies
Qt6, OpenCV 4, Boost (thread, system), libxml2, Potrace, Poppler, Cairo, GLib, Sparkle (macOS auto-updates)

## Coding Conventions

- Use modern C++17 features (smart pointers, structured bindings, etc.)
- Keep business logic out of widgets and QML — maintain low coupling with Qt Framework
- The `develop` branch is the main integration branch
- Internationalization files are in `i18n/` (English, Traditional Chinese, Japanese)

## Run Modes

The application supports three run modes controlled via CLI args:
1. GUI (default) — full windowed application
2. CLI (`cli <file>`) — headless SVG processing
3. Daemon (`--daemon`) — WebSocket server mode
