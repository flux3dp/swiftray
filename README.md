<p align="center">
  <img
    alt="swiftray library logo"
    src="images/icon.png"
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

## Requirements

- Compilers must support C++17 standards.
- Boost 1.7.0
- Qt Framework 5.15
- Qt Creator
- Qt Framework and Creator can be installed via [online installer](https://www.qt.io/download-open-source)
- OpenCV 4
- Potrace
- libxml2
- libiconv (Windows)
- icu4c (MacOS)

## Building

### CMake

This project uses CMake to build.

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make -j12
```

### Qt Creator (QMake)

Open the .pro project file in the root directory, and click run.

## Deploy

### MacOS
We can use built-in tool `macdeployqt` to deploy the app bundle generated by QMake
```bash
$ cd <bin folder in Qt>
$ ./macdeployqt <absolute path>/build-swiftray-Desktop_Qt_5_15_2_clang_64bit-Release/Swiftray.app -qmldir=<absolute path>/swiftray//src/windows/qml -dmg
```

## Coding Guides

1. Use Modern C++ as possible as you can.
2. Reduce logic implementation in widgets and QML codes, to maintain low coupling with Qt Framework.

## Document

Run `$ doxygen Doxygen` and view docs/index.html

## Affiliation

Swiftray is community developed, commercially supported for long-term development.

Swiftray is brought to you by the development team at FLUX Inc.


## License

Swiftray is GNU General Public License v3.0 licensed.
