QT += quick
QT += quickwidgets
QT += opengl

CONFIG += c++17
CONFIG += optimize_full
LIBS += -L"/usr/local/lib"
LIBS += -L"/usr/local/opt/libxml2/lib"
LIBS += -lxml2
INCLUDEPATH += $$PWD/third_party
INCLUDEPATH += $$PWD/src
INCLUDEPATH += /usr/local/include/
INCLUDEPATH += /usr/local/opt/libxml2/include

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        src/canvas/canvas_data.cpp \
        src/canvas/layer.cpp \
        src/canvas/transform_box.cpp \
        src/window/main.cpp \
        src/window/mainwindow.cpp \
        src/shape/shape.cpp \
        src/shape/group_shape.cpp \
        src/shape/path_shape.cpp \
        src/vdoc.cpp \
        src/canvas/vcanvas.cpp \
        src/parser/svgpp_parser.cpp \
        src/parser/svgpp_context.cpp \
        src/parser/svgpp_impl.cpp

RESOURCES += qml.qrc

TRANSLATIONS += \
    vecty_zh_TW.ts
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
    src/canvas/canvas_data.hpp \
    src/canvas/layer.hpp \
    src/canvas/transform_box.hpp \
    src/window/mainwindow.h \
    src/window/osxwindow.h \
    src/shape/group_shape.h \
    src/shape/path_shape.h \
    src/canvas/vcanvas.h \
    src/vdoc.h \
    src/shape/shape.hpp \
    src/parser/svgpp_common.hpp \
    src/parser/svgpp_parser.hpp \
    src/parser/svgpp_context.hpp \
    src/parser/svgpp_impl.hpp \
    src/parser/svgpp_color_factory.hpp

win32:CONFIG(release, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_system
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_systemd

INCLUDEPATH += C:/cygwin64/usr/include
DEPENDPATH += C:/cygwin64/usr/include

DESTDIR = /Users/simon/Dev/vecty/build
OBJECTS_DIR = /Users/simon/build/vecty/.obj
MOC_DIR = /Users/simon/build/vecty/.moc
RCC_DIR = /Users/simon/build/vecty/.rcc
UI_DIR = /Users/simon/build/vecty/.ui

FORMS += \
    src/window/mainwindow.ui

DISTFILES += \
    ../../Downloads/icons8-cursor-48.png \
    images/select.png

OBJECTIVE_SOURCES += src/window/osxwindow.mm
