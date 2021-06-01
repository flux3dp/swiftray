all: qmake cpp run
	mkdir -p build
	cd build; qmake ../vecty.pro -spec macx-clang CONFIG+=x86_64 CONFIG-=qml_debug CONFIG+=qtquickcompiler CONFIG-=separate_debug_info && /usr/bin/make qmake_all
	cd build; make -j16
qmake:
	mkdir -p build
	cd build; qmake ../vecty.pro -spec macx-clang CONFIG+=x86_64 CONFIG-=qml_debug CONFIG+=qtquickcompiler CONFIG-=separate_debug_info && /usr/bin/make qmake_all
cpp:
	mkdir -p build
	cd build; make -j16
run: 
	./build/vecty.app/Contents/MacOS/vecty
clean:
	rm -rf build