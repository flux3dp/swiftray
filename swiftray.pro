QT += quick
QT += quickwidgets
QT += widgets
QT += opengl
QT += gui
QT += gui-private
QT += svg
QT += svg-private
ios {
} else {
QT += serialport
}

QMAKE_TARGET_BUNDLE_PREFIX = com.flux
TARGET = Swiftray
ICON=images/icon.icns
CONFIG += c++17
win32 {
    # libxml2, potrace, boost, opencv
    LIBS += -L$${MINGW64_PATH}/lib
    # resolve __imp_WSAStartup & __imp_WSACleanup undefined issue
    LIBS += -lws2_32
    # resolve WinSock.h already included issue
    DEFINES+=WIN32_LEAN_AND_MEAN
}
macx{
    _BOOST_PATH = "/usr/local/Cellar/boost/1.76.0"
    LIBS += -L"/usr/lib"
    LIBS += -L"/usr/local/lib"
    LIBS += -L"/usr/local/opt/libxml2/lib"
    LIBS += -L"/usr/local/opt/opencv/lib"
}
ios {
    LIBS += -framework Foundation -framework UIKit
}
LIBS += -lxml2
LIBS += -lboost_thread-mt
LIBS += -lboost_system-mt
LIBS += -lopencv_core
LIBS += -lopencv_imgproc
LIBS += -lpotrace

INCLUDEPATH += $$PWD/third_party
INCLUDEPATH += $$PWD/src
win32 {
    # boost, libxml2, potrace
    INCLUDEPATH += $${MINGW64_PATH}/include
    INCLUDEPATH += $${MINGW64_PATH}/include/libxml2
    INCLUDEPATH += $${MINGW64_PATH}/include/opencv4
}
macx{
    INCLUDEPATH += /usr/local/include
    INCLUDEPATH += /usr/local/opt/libxml2/include/libxml2/
    INCLUDEPATH += /usr/local/opt/opencv/include/opencv4
    INCLUDEPATH += "$${_BOOST_PATH}/include/"
}

# Remove -Wall and -Wextra flag
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
macx{
    # flag for clang only
    QMAKE_CXXFLAGS += -ferror-limit=1
}
QMAKE_CXXFLAGS += -ftemplate-backtrace-limit=12
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
SOURCES += \
    $$files(src/*.cpp) \
    $$files(src/canvas/*.cpp) \
    $$files(src/canvas/controls/*.cpp) \
    $$files(src/connection/*.cpp) \
    $$files(src/connection/QAsyncSerial/*.cpp) \
    $$files(src/gcode/*.cpp) \
    $$files(src/motion_controller_job/*.cpp) \
    $$files(src/parser/*.cpp) \
    $$files(src/settings/*.cpp) \
    $$files(src/shape/*.cpp) \
    $$files(src/widgets/*.cpp) \
    $$files(src/widgets/panels/*.cpp) \
    $$files(src/widgets/components/*.cpp) \
    $$files(src/windows/*.cpp) \
    $$files(third_party/QxPotrace/src/qxpotrace.cpp) \
    src/widgets/components/graphicitems/resizeable-rect-item.cpp \
    third_party/clipper/clipper.cpp

RESOURCES += qml.qrc
TRANSLATIONS += \
    i18n/zh-Hant-TW.ts
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
    $$files(src/gcode/*.h) \
    $$files(src/motion_controller_job/*.h) \
    $$files(src/parser/*.h) \
    $$files(src/parser/generators/*.h) \
    $$files(src/settings/*.h) \
    $$files(src/shape/*.h) \
    $$files(src/widgets/*.h) \
    $$files(src/widgets/panels/*.h) \
    $$files(src/widgets/components/*.h) \
    $$files(src/windows/*.h) \
    src/gcode/generators/dirty-area-outline-generator.h \
    $$files(third_party/QxPotrace/include/qxpotrace.h) \
    third_party/clipper/clipper.hpp
ios {
    HEADERS += \
            src/widgets/components/ios-image-picker.h \
}

FORMS += \
    src/widgets/components/layer-list-item.ui \
    src/widgets/components/task-list-item.ui \
    src/widgets/panels/doc-panel.ui \
    src/widgets/panels/font-panel.ui \
    src/widgets/panels/jogging-panel.ui \
    src/widgets/panels/layer-panel.ui \
    src/widgets/panels/layer-params-panel.ui \
    src/widgets/panels/spooler-panel.ui \
    src/widgets/panels/transform-panel.ui \
    src/windows/job-dashboard-dialog.ui \
    src/windows/machine-manager.ui \
    src/windows/machine-monitor.ui \
    src/windows/new-machine-dialog.ui \
    src/windows/preferences-window.ui \
    src/windows/preset-manager.ui \
    src/windows/preview-window.ui \
    src/windows/gcode-player.ui \
    src/windows/mainwindow.ui \
    src/windows/image-trace-dialog.ui \
    src/windows/path-offset-dialog.ui \
    src/widgets/panels/image-panel.ui \
    src/widgets/components/color-picker-button.ui \
    src/windows/image-crop-dialog.ui \
    src/windows/image-sharpen-dialog.ui
ios {
OBJECTIVE_SOURCES += src/widgets/components/ios-image-picker.mm
} else {
OBJECTIVE_SOURCES += src/windows/osxwindow.mm
}
TR_EXCLUDE += $$PWD/third_party/* \
             /usr/local/include/* \
             /usr/local/opt/libxml2/include/* \
             /usr/local/include/boost/*

QML_IMPORT_PATH = src/windows \
                  src/windows/qml
