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

CONFIG += c++17
LIBS += -L"/usr/lib"
LIBS += -L"/usr/local/lib"
LIBS += -L"/usr/local/opt/libxml2/lib"
LIBS += -lxml2
ios {
    LIBS += -framework Foundation -framework UIKit
}
INCLUDEPATH += $$PWD/third_party
INCLUDEPATH += $$PWD/src
INCLUDEPATH += /usr/local/include/
INCLUDEPATH += /usr/local/opt/libxml2/include

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
        $$files(src/canvas/controls/*.cpp) \
        $$files(src/shape/*.cpp) \
        $$files(src/widgets/*.cpp) \
        $$files(src/widgets/panels/*.cpp) \
        $$files(src/widgets/components/*.cpp) \
        $$files(src/parser/*.cpp) \
        $$files(src/canvas/*.cpp) \
        $$files(src/gcode/*.cpp) \
        $$files(src/windows/*.cpp) \
        $$files(src/settings/*.cpp) \
        src/document.cpp \
        src/layer.cpp \
        src/command.cpp \
        src/clipboard.cpp \
        src/connection/base-job.cpp

ios {
} else {
    SOURCES += \
            src/connection/serial-job.cpp
}

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
    $$files(src/canvas/*.h) \
    $$files(src/canvas/controls/*.h) \
    $$files(src/shape/*.h) \
    $$files(src/parser/*.h) \
    $$files(src/parser/generators/*.h) \
    $$files(src/gcoder/*.h) \
    $$files(src/widgets/*.h) \
    $$files(src/widgets/panels/*.h) \
    $$files(src/windows/*.h) \
    $$files(src/*.h) \
    src/connection/base-job.h \
    src/widgets/components/canvas-text-edit.h \
    src/widgets/components/layer-list-item.h \
    src/widgets/components/qdoublespinbox2.h \
    src/widgets/components/qfontcombobox2.h \
    src/widgets/components/task-list-item.h

ios {
    HEADERS += \
            src/widgets/components/ios-image-picker.h \

} else {
    HEADERS += \
            src/connection/serial-job.h
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
    src/widgets/panels/layer-panel.ui \
    src/widgets/panels/layer-params-panel.ui \
    src/widgets/panels/spooler-panel.ui \
    src/widgets/panels/transform-panel.ui \
    src/windows/machine-manager.ui \
    src/windows/machine-monitor.ui \
    src/windows/new-machine-dialog.ui \
    src/windows/preferences-window.ui \
    src/windows/preset-manager.ui \
    src/windows/preview-window.ui \
    src/windows/gcode-player.ui \
    src/windows/mainwindow.ui

ios {
OBJECTIVE_SOURCES += src/widgets/components/ios-image-picker.mm
} else {
OBJECTIVE_SOURCES += src/windows/osxwindow.mm
}

DISTFILES += \
    src/windows/SetupScreen.qml \
    src/windows/SetupScreenForm.ui.qml

TR_EXCLUDE += $$PWD/third_party/* \
             /usr/local/include/* \
             /usr/local/opt/libxml2/include/* \
             /usr/local/include/boost/* \
             /Users/simon/Dev/qt5/qtbase/*
