QT += quick
QT += quickwidgets
QT += widgets 
QT += opengl
QT += gui
QT += gui-private
QT += svg
QT += svg-private

CONFIG += c++17
LIBS += -L"/usr/local/lib"
LIBS += -L"/usr/local/opt/libxml2/lib"
LIBS += -lxml2
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
        $$files(src/parser/*.cpp) \
        $$files(src/canvas/*.cpp) \
        $$files(src/gcode/*.cpp) \
        src/undo.cpp \
        src/document.cpp \
        src/layer.cpp \
        src/widgets/font-panel.cpp \
        src/widgets/preset-manager.cpp \
        src/window/gcode-player.cpp \
        src/window/main.cpp \
        src/window/mainwindow.cpp \

RESOURCES += qml.qrc

TRANSLATIONS += \
    beambird_zh_TW.ts
CONFIG += lrelease
CONFIG += embed_translations

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    src/document.h \
    src/layer.h \
    src/command.h \
    $$files(src/canvas/*.h) \
    $$files(src/canvas/controls/*.h) \
    $$files(src/shape/*.h) \
    $$files(src/parser/*.h) \
    $$files(src/parser/generators/*.h) \
    $$files(src/gcoder/*.h) \
    $$files(src/widgets/*.h) \
    src/widgets/font-panel.h \
    src/widgets/preset-manager.h \
    src/window/gcode-player.h \
    src/window/mainwindow.h \
    src/window/osxwindow.h

win32:CONFIG(release, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_system
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_systemd

INCLUDEPATH += C:/cygwin64/usr/include
DEPENDPATH += C:/cygwin64/usr/include

FORMS += \
    src/widgets/font-panel.ui \
    src/widgets/layer-list-item.ui \
    src/widgets/layer-params-panel.ui \
    src/widgets/preset-manager.ui \
    src/widgets/preview-window.ui \
    src/widgets/transform-panel.ui \
    src/widgets/gcode-player.ui \
    src/window/mainwindow.ui

OBJECTIVE_SOURCES += src/window/osxwindow.mm
