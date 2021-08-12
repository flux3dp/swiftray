# Swiftray

Swiftray is a free and open-sourced software for grbl-based laser cutters and engravers. 

**Features**

- Layer controls - cut and engrave simultaneously
- Divide by color - efficient workflows for third party design software
- Hack as you like - you can modify all codes to match you self-built lasers
- Blazing performance - written in C++
- Low memory usage - even runnable on embedded system
- Cross-platform - compiles on Windows, macOS, and Linux

## Dependencies

- Compilers must support C++17 standards.
- Boost 1.7.0
- Qt Framework 5.15
- Qt Creator
- Qt Framework and Creator can be installed via [online installer](https://www.qt.io/download-open-source)
- OpenCV 4

## Building

### CMake

This project uses CMake to build.

```
$> mkdir build
$> cd build
$> cmake ..
$> make -j12
```

### Qt Creator

Open the .pro project file in the root directory, and click run.

## Coding Guides

1. Use Modern C++ as possible as you can.
2. Reduce logic implementation in widgets and QML codes, to maintain low coupling with Qt Framework.

## Document

Run `$> doxygen Doxygen` and view docs/index.html

## Affiliation

Swiftray is community developed, commercially supported for long-term development.

Swiftray is brought to you by the development team at FLUX Inc.
