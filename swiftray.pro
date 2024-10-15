CONFIG += USE_QT6
CONFIG += static

# === Define QT Libraries ===
QT += quick
QT += quickwidgets
QT += widgets
QT += opengl
QT += gui
QT += gui-private
QT += quickcontrols2
QT += svg
QT += svg-private
QT += websockets
QT += openglwidgets
QT += core5compat
QT += serialport
QT += openglwidgets
QT += core5compat

# === Define Project Name and Versions ===
QMAKE_TARGET_BUNDLE_PREFIX = com.flux
TARGET = Swiftray

VERSION_MAJOR = 1
VERSION_MINOR = 3
VERSION_BUILD = 0
VERSION_SUFFIX = "" # empty string or "-beta.X"
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}$${VERSION_SUFFIX}

win32 {
  VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}
  #$${VERSION_SUFFIX}
}

# === Compilation Config ===
DEFINES += "QT6"
QMAKE_INFO_PLIST = Info.plist
ICON=resources/images/icon.icns
RC_ICONS = resources/images/icon.ico
CONFIG += c++17
CONFIG += no_keywords

win32 {
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -Os
}

# === Import Other Libraries ===

win32 {
    # libxml2, potrace, boost, opencv
    win32-msvc {
        LIBS += "C:\Dev\libraries\libxml2\lib\libxml2.dll.a"
        LIBS += "C:\Dev\libraries\potrace\libpotrace.dll.a"
        LIBS += -LC:\Dev\libraries\potrace
        LIBS += -LC:\Dev\boost_1_78_0_msvc\lib64-msvc-14.1
        LIBS += -LC:\tools\opencv\build\x64\vc14\lib
        win32:CONFIG(release, debug|release): LIBS += -LC:\Dev\opencv_454_msvc\build\x64\vc14\lib -lopencv_world454
        else:win32:CONFIG(debug, debug|release): LIBS += -LC:\Dev\opencv_454_msvc\build\x64\vc14\lib -lopencv_world454d
        LIBS += -lboost_thread-vc141-mt-x64-1_78
        LIBS += -lboost_system-vc141-mt-x64-1_78
        LIBS += -L$$PWD\third_party\sentry-native\install\lib -lsentry
    }
    win32-g++ {
        # MINGW
        LIBS += -L$$(MINGW64_PATH)/lib
        LIBS += -lboost_thread-mt
        LIBS += -lboost_system-mt
        LIBS += -lopencv_core
        LIBS += -lopencv_imgproc
        LIBS += -lopencv_flann
        LIBS += -lxml2
        LIBS += -lpotrace
        LIBS += -llibpotrace
        LIBS += -L$$PWD\third_party\sentry-native\install\lib -lsentry
    }
    # resolve __imp_WSAStartup & __imp_WSACleanup undefined issue
    LIBS += -lws2_32
    # resolve WinSock.h already included issue
    DEFINES += WIN32_LEAN_AND_MEAN
}

macx{
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
    LIBS += -L"/usr/local/lib"

    # Mac M1
    contains(QMAKE_HOST.arch, arm64) {
        _BOOST_PATH = "/opt/homebrew/opt/boost"
        LIBS += -L"/opt/homebrew/opt/boost/lib"
        LIBS += -L"/opt/homebrew/opt/libxml2/lib"
        LIBS += -L"/opt/homebrew/opt/opencv/lib"
        LIBS += -L"/opt/homebrew/opt/glib/lib"
        LIBS += -L"/opt/homebrew/opt/poppler/lib"
        LIBS += -L"/opt/homebrew/opt/cairo/lib"
        LIBS += -L"/opt/homebrew/opt/potrace/lib"
    } lese {
        # Mac Intel
        _BOOST_PATH = "/usr/local/opt/boost/"
        LIBS += -L"/usr/lib"
        LIBS += -L"/usr/local/opt/libxml2/lib"
        LIBS += -L"/usr/local/opt/opencv/lib"

        LIBS += -lboost_thread-mt
        LIBS += -lboost_system-mt
        LIBS += -lopencv_core
        LIBS += -lopencv_imgproc
        LIBS += -lopencv_flann
        LIBS += -lxml2
        LIBS += -lpotrace
        LIBS += -lglib-2.0
        LIBS += -lgobject-2.0
        LIBS += -lpoppler-glib
        LIBS += -lpoppler
        LIBS += -lcairo
        LIBS += -L$$PWD/third_party/sentry-native/install/lib -lsentry
    }
}

unix:!macx{
    # TODO: Linux
    LIBS += -lboost_thread-mt
    LIBS += -lboost_system-mt
    LIBS += -lopencv_core
    LIBS += -lopencv_imgproc
    LIBS += -lopencv_flann
    LIBS += -lxml2
    LIBS += -lpotrace
    LIBS += -llibpotrace
}

message($$OUT_PWD)
versionconfig.input = $$PWD/qmake/qmakeconfig.h.in
versionconfig.output = $$OUT_PWD/config.h
QMAKE_SUBSTITUTES += versionconfig

# === Add Header Paths ===
INCLUDEPATH += $$OUT_PWD/config.h
INCLUDEPATH += $$PWD/third_party
INCLUDEPATH += $$PWD/src

win32-msvc {
    INCLUDEPATH += C:\Dev\boost_1_78_0_msvc
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\include
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\include\opencv2
    INCLUDEPATH += $$PWD\third_party\libpotrace\potrace-1.16\src
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\include\libxml2
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\include\libconv
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\include\glib-2.0
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\include\cairo
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\lib\glib-2.0\include
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\lib
    INCLUDEPATH += $$PWD\vcpkg_installed\x64-windows\include\poppler
    INCLUDEPATH += $$PWD\third_party
    INCLUDEPATH += $$PWD\third_party\sentry-native\install\include
}

macx{
    INCLUDEPATH += /usr/local/include
    INCLUDEPATH += "$${_BOOST_PATH}/include/"
    INCLUDEPATH += /usr/local/opt/icu4c/include
    INCLUDEPATH += $$PWD/third_party/sentry-native/install/include
    contains(QMAKE_HOST.arch, arm64) {
        # Mac M1
       INCLUDEPATH += /opt/homebrew/opt/libxml2/include/libxml2/
       INCLUDEPATH += /opt/homebrew/opt/opencv/include/opencv4
       INCLUDEPATH += /opt/homebrew/include/poppler/glib
       INCLUDEPATH += /opt/homebrew/include/glib-2.0
       INCLUDEPATH += /opt/homebrew/opt/glib/lib/glib-2.0/include
       INCLUDEPATH += /opt/homebrew/opt/cairo/include/cairo
       INCLUDEPATH += /opt/homebrew/opt/poppler/include/poppler
       INCLUDEPATH += /opt/homebrew/opt/potrace/include
    } else {
        # Mac Intel
        INCLUDEPATH += /usr/local/opt/libxml2/include/libxml2/
        INCLUDEPATH += /usr/local/opt/opencv/include/opencv4
        INCLUDEPATH += /usr/local/include/poppler/glib
        INCLUDEPATH += /usr/local/include/glib-2.0
        INCLUDEPATH += /usr/local/opt/glib/lib/glib-2.0/include
        INCLUDEPATH += /usr/local/opt/cairo/include/cairo
        INCLUDEPATH += /usr/local/opt/poppler/include/poppler
    }

}

# === CXX Config ===
# Remove -Wall and -Wextra flag
win32 {
    win32-g++ {
        QMAKE_CFLAGS_WARN_ON -= -Wall
        QMAKE_CXXFLAGS_WARN_ON -= -Wall
        QMAKE_CFLAGS_WARN_ON -= -Wextra
        QMAKE_CXXFLAGS_WARN_ON -= -Wextra
        QMAKE_CXXFLAGS += -Wall
        # QMAKE_CXXFLAGS += -Wextra
        QMAKE_CXXFLAGS += -Wno-unused-local-typedef
        QMAKE_CXXFLAGS += -Wno-unused-variable
        QMAKE_CXXFLAGS += -Wno-reorder-ctor
        QMAKE_CXXFLAGS += -Wno-deprecated-declarations
        QMAKE_CXXFLAGS += -ftemplate-backtrace-limit=12
    }
    win32-msvc {
        debug:QMAKE_CXXFLAGS += -bigobj
    }
}

unix {
    QMAKE_CFLAGS_WARN_ON -= -Wall
    QMAKE_CXXFLAGS_WARN_ON -= -Wall
    QMAKE_CFLAGS_WARN_ON -= -Wextra
    QMAKE_CXXFLAGS_WARN_ON -= -Wextra
    QMAKE_CXXFLAGS += -Wall
    # QMAKE_CXXFLAGS += -Wextra
    QMAKE_CXXFLAGS += -Wno-unused-local-typedef
    QMAKE_CXXFLAGS += -Wno-unused-variable
    QMAKE_CXXFLAGS += -Wno-reorder-ctor
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations
    QMAKE_CXXFLAGS += -ftemplate-backtrace-limit=12
}

macx{
    # flag for clang only
    QMAKE_CXXFLAGS += -ferror-limit=1
    QMAKE_CXXFLAGS += -ftemplate-backtrace-limit=12
}
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# === Define Sources ===
SOURCES += \
    $$files(src/*.cpp) \
    $$files(src/canvas/*.cpp) \
    $$files(src/canvas/controls/*.cpp) \
    $$files(src/connection/*.cpp) \
    $$files(src/connection/QAsyncSerial/*.cpp) \
    $$files(src/toolpath_exporter/*.cpp) \
    $$files(src/settings/*.cpp) \
    $$files(src/shape/*.cpp) \
    $$files(src/widgets/*.cpp) \
    $$files(src/widgets/panels/*.cpp) \
    $$files(src/widgets/components/*.cpp) \
    $$files(src/windows/*.cpp) \
    $$files(src/executor/*.cpp) \
    $$files(src/executor/machine_job/*.cpp) \
    $$files(src/executor/operation_cmd/*.cpp) \
    $$files(src/machine/*.cpp) \
    $$files(src/periph/*.cpp) \
    $$files(src/periph/motion_controller/*.cpp) \
    $$files(src/common/*.cpp) \
    $$files(src/server/*.cpp) \
    $$files(src/debug/*.cpp) \
    $$files(third_party/QxPotrace/src/qxpotrace.cpp) \
    $$files(src/parser/*.cpp) \
    $$files(src/parser/mysvg/*.cpp) \
    $$files(src/parser/dxf_rs/debug/*.cpp) \
    $$files(src/parser/dxf_rs/engine/*.cpp) \
    $$files(src/parser/dxf_rs/math/*.cpp) \
    $$files(src/parser/dxf_rs/information/*.cpp) \
    $$files(src/parser/dxf_rs/filters/*.cpp) \
    $$files(src/parser/dxf_rs/fileio/*.cpp) \
    $$files(src/parser/dxf_rs/muparser/*.cpp) \
    $$files(src/parser/dxf_rs/jwwlib/*.cpp) \
    $$files(src/utils/*.cpp) \
    src/widgets/components/graphicitems/resizeable-rect-item.cpp \
    third_party/clipper/clipper.cpp \
    third_party/liblcs/lcsExpr.cpp \
    $$files(third_party/libdxfrw/*.cpp) \
    $$files(third_party/libdxfrw/intern/*.cpp)

SOURCES -= src/executor/operation_cmd/sync_exec_cmd.cpp

RESOURCES += qml.qrc
RESOURCES += sparkle.qrc
TRANSLATIONS += \
    i18n/zh-Hant-TW.ts \
    i18n/ja-JP.ts

CONFIG += lrelease
CONFIG += embed_translations

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =
# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path):INSTALLS += target

HEADERS += \
    $$files(src/*.h) \
    $$files(src/canvas/*.h) \
    $$files(src/canvas/controls/*.h) \
    $$files(src/connection/*.h) \
    $$files(src/connection/QAsyncSerial/*.h) \
    $$files(src/toolpath_exporter/*.h) \
    $$files(src/parser/*.h) \
    $$files(src/parser/generators/*.h) \
    $$files(src/settings/*.h) \
    $$files(src/shape/*.h) \
    $$files(src/widgets/*.h) \
    $$files(src/widgets/panels/*.h) \
    $$files(src/widgets/components/*.h) \
    $$files(src/windows/*.h) \
    $$files(src/server/*.h) \
    $$files(src/debug/*.h) \
    $$files(src/parser/mysvg/*.h) \
    $$files(src/parser/dxf_rs/debug/*.h) \
    $$files(src/parser/dxf_rs/math/*.h) \
    $$files(src/parser/dxf_rs/information/*.h) \
    $$files(src/parser/dxf_rs/engine/*.h) \
    $$files(src/parser/dxf_rs/fileio/*.h) \
    $$files(src/parser/dxf_rs/muparser/*.h) \
    $$files(src/parser/dxf_rs/jwwlib/*.h) \
    src/config.h \
    src/toolpath_exporter/generators/dirty-area-outline-generator.h \
    $$files(src/executor/*.h) \
    $$files(src/executor/machine_job/*.h) \
    $$files(src/executor/operation_cmd/*.h) \
    $$files(src/machine/*.h) \
    $$files(src/periph/*.h) \
    $$files(src/periph/motion_controller/*.h) \
    $$files(src/common/*.h) \
    $$files(third_party/QxPotrace/include/qxpotrace.h) \
    src/widgets/panels/laser-panel.h \
    src/windows/rotary_setup.h \
    third_party/clipper/clipper.hpp \
    $$files(third_party/libdxfrw/intern/*.h) \
    $$files(third_party/libdxfrw/*.h)

FORMS += \
    src/widgets/components/layer-list-item.ui \
    src/widgets/components/task-list-item.ui \
    src/widgets/panels/doc-panel.ui \
    src/widgets/panels/font-panel.ui \
    src/widgets/panels/jogging-panel.ui \
    src/widgets/panels/laser-panel.ui \
    src/widgets/panels/layer-panel.ui \
    src/widgets/panels/layer-params-panel.ui \
    src/widgets/panels/spooler-panel.ui \
    src/widgets/panels/transform-panel.ui \
    src/windows/about-window.ui \
    src/windows/consoledialog.ui \
    src/windows/job-dashboard-dialog.ui \
    src/windows/machine-manager.ui \
    src/windows/machine-monitor.ui \
    src/windows/preferences-window.ui \
    src/windows/preset-manager.ui \
    src/windows/preview-window.ui \
    src/windows/gcode-panel.ui \
    src/windows/mainwindow.ui \
    src/windows/image-trace-dialog.ui \
    src/windows/path-offset-dialog.ui \
    src/widgets/panels/image-panel.ui \
    src/widgets/components/color-picker-button.ui \
    src/windows/image-crop-dialog.ui \
    src/windows/image-sharpen-dialog.ui \
    src/windows/privacy_window.ui \
    src/windows/rotary_setup.ui

macx {
    OBJECTIVE_SOURCES += src/windows/osxwindow.mm
    OBJECTIVE_SOURCES += src/osx/disable-app-nap.mm
}

TR_EXCLUDE += $$PWD/third_party/* \
             /usr/local/include/* \
             /usr/local/opt/libxml2/include/* \
             /usr/local/include/boost/*

QML_IMPORT_PATH = src/windows \
                  src/windows/qml


# === macOS Bundle Config ===
# Mac M1
# QMAKE_APPLE_DEVICE_ARCHS=arm64

macx{
  # Copy additional files to bundle
  BUNDLE_FRAMEWORKS_FILES.files += $$PWD/third_party/sentry-native/install/lib/libsentry.dylib
  BUNDLE_FRAMEWORKS_FILES.path = Contents/Frameworks
  QMAKE_BUNDLE_DATA += BUNDLE_FRAMEWORKS_FILES
  
  BUNDLE_ADDITIONAL_EXEC_FILES.files += $$PWD/third_party/sentry-native/install/bin/crashpad_handler \
                                        $$files($$PWD/third_party/liblcs/lib/*.dylib)
  BUNDLE_ADDITIONAL_EXEC_FILES.path = Contents/MacOS
  QMAKE_BUNDLE_DATA += BUNDLE_ADDITIONAL_EXEC_FILES
}

DISTFILES +=
