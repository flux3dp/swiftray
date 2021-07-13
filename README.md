# BeamBird

Powerful and fast laser engraver/cutter tool based on C++.

## Dependencies

- Compilers must support C++17 standards.
- Qt Framework 5.15
- Qt Creator
- Qt Framework and Creator can be installed via [online installer](https://www.qt.io/download-open-source)

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

# Document

Run `$> doxygen Doxygen` and view docs/index.html