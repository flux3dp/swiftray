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
_BOOST_PATH = "/usr/local/Cellar/boost/1.76.0"
CONFIG += c++17
LIBS += -L"/usr/lib"
LIBS += -L"/usr/local/lib"
LIBS += -L"/usr/local/opt/libxml2/lib"
LIBS += -L"/usr/local/opt/opencv/lib"
LIBS += -lxml2
ios {
    LIBS += -framework Foundation -framework UIKit
}
LIBS += -lboost_thread-mt
LIBS += -lopencv_core
LIBS += -lopencv_imgproc
INCLUDEPATH += $$PWD/third_party
INCLUDEPATH += $$PWD/src
INCLUDEPATH += /usr/local/include/
INCLUDEPATH += /usr/local/opt/libxml2/include
INCLUDEPATH += /usr/local/opt/opencv/include/opencv4
INCLUDEPATH += "$${_BOOST_PATH}/include/"
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
QMAKE_CXXFLAGS += -ferror-limit=1
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

win32:CONFIG(release, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_system
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_systemd
win32 {
INCLUDEPATH += C:/cygwin64/usr/include
DEPENDPATH += C:/cygwin64/usr/include
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
win32:CONFIG(release, debug|release): LIBS += -L/usr/local/lib/release/ -lpotrace
else:win32:CONFIG(debug, debug|release): LIBS += -L/usr/local/lib/debug/ -lpotrace
else:unix: LIBS += -L/usr/local/lib/ -lpotrace
INCLUDEPATH += /usr/local/include
DEPENDPATH += /usr/local/include
win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += /usr/local/lib/release/libpotrace.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += /usr/local/lib/debug/libpotrace.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += /usr/local/lib/release/potrace.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += /usr/local/lib/debug/potrace.lib
else:unix: PRE_TARGETDEPS += /usr/local/lib/libpotrace.a
