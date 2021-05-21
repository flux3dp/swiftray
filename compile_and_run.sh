mkdir -p build
cd ./build
/Users/simon/Qt/5.15.1/clang_64/bin/qmake /Users/simon/Dev/vecty/vecty.pro -spec macx-clang CONFIG+=x86_64 CONFIG-=qml_debug CONFIG+=qtquickcompiler CONFIG-=separate_debug_info && /usr/bin/make qmake_all
make -j16 in /Users/simon/Dev/build-vecty-Desktop_Qt_5_15_1_clang_64bit-Release
./vecty.app/Contents/MacOS/vecty