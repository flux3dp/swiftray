<p align="center">
  <img
    alt="swiftray library logo"
    src="resources/images/icon.png"
    height="300"
    style="margin-top: 20px; margin-bottom: 20px;"
  />
</p>


# Swiftray

Swiftray is a free and open-sourced software for grbl-based laser cutters and engravers. 

**Features**

- Layer controls - cut and engrave simultaneously
- Divide by color - efficient workflows for third party design software
- Hack as you like - you can modify all codes to match you self-built lasers
- Blazing performance - written in C++
- Low memory usage - even runnable on embedded system
- Cross-platform - compiles on Windows and MacOS

## Dependencies

- Compilers must support C++17 standards.
- Qt Framework 6.7.2
- Qt Framework and Creator can be installed via [online installer](https://www.qt.io/download-open-source)
- OpenCV 4
- Boost 1.8
- Potrace
- libxml2
- libiconv (Windows)
- icu4c (MacOS)
- poppler
- glib
- cairo

## Setup
### 1. Clone the repo and checkout submodules
```
git clone https://github.com/flux3dp/swiftray.git
cd swiftray
git submodule update --init --recursive
```

### 2. Build sentry-native

#### macOS
```
cd third_party/sentry-native
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DSENTRY_BACKEND=crashpad \
  -DSENTRY_INTEGRATION_QT=YES \
  -DCMAKE_PREFIX_PATH=<PATH_TO_QT>/lib/cmake
cmake --build build --config RelWithDebInfo --parallel
cmake --install build --prefix install
```
#### Windows
```
# The process can be skipped since the sentry-native can be installed via vcpkg
```

### 3. Build libpotrace from source and handle it with Conan (for Windows)
libpotrace must be compiled in msys or cygwin environment.

Take msys2 for example, install the following packages:
```
pacman -S base-devel
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-autotools
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-zlib
pacman -S mingw-w64-x86_64-python-conan
```
then
```
$ cd third_party/libpotrace
$ curl https://potrace.sourceforge.net/download/1.16/potrace-1.16.tar.gz -o potrace-1.16.tar.gz
$ tar -xzvf potrace-1.16.tar.gz
$ conan install . --build=missing
$ conan build .
$ conan create . user/testing
```

## Building

### macOS
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make -j12
```

### Windows
1. Install the latest Visual Studio (tested in 2019) with vcpkg
2. For local development, set env QML2_IMPORT_PATH to `/path/to/qml/dlls` if debug mode cannot load qml plugins

## Deployment

### macOS
Use the following command to make a *distributable* app bundle.
```
cmake --build . --target swiftray_app_bundle -j8
```
### Windows
Windeployqt is a tool that will gather all the required deployment files for your application. It will copy the necessary Qt libraries, plugins, and QML files to the directory where the application is located. It is necessary to run after compiling the application to properly run the application.
```
$ <Qt_Installed_Dir>/6.7.2/msvc2019_64/bin/windeployqt.exe --qmldir src/windows/qml --compiler-runtime build/bin/Swiftray.exe
```

## Coding Guides

1. Use Modern C++ as possible as you can.
2. Reduce logic implementation in widgets and QML codes, to maintain low coupling with Qt Framework.

## Document

Run `$ doxygen Doxygen` and view docs/index.html

## Affiliation

Swiftray is community developed, commercially supported for long-term development.

Swiftray is original brought to you by the development team at FLUX Inc.


## License

Swiftray is GNU General Public License v3.0 licensed.
