QT += quick
QT += opengl

CONFIG += c++17
CONFIG += optimize_full
LIBS += -L"/usr/local/lib"
LIBS += -L"/usr/local/opt/libxml2/lib"
LIBS += -lxml2
INCLUDEPATH += $$PWD/third_party
INCLUDEPATH += /usr/local/include/
INCLUDEPATH += /usr/local/opt/libxml2/include

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        vcontext.cpp \
        vdoc.cpp \
        vcanvas.cpp \
        parser/svgpp_impl.cpp

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
    vcanvas.h \
    vcontext.h \
    vdoc.h \
    parser/svgpp_common.hpp \
    parser/svgpp_impl.hpp \
    parser/svgpp_color_factory.hpp

win32:CONFIG(release, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_system
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/cygwin64/lib/ -lboost_systemd

INCLUDEPATH += C:/cygwin64/usr/include
DEPENDPATH += C:/cygwin64/usr/include

DESTDIR = build/vecty/
OBJECTS_DIR = build/vecty/.obj
MOC_DIR = build/vecty/.moc
RCC_DIR = build/vecty/.rcc
UI_DIR = build/vecty/.ui