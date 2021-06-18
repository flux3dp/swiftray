cmake_minimum_required(VERSION 3.15)
project(vecty)

FIND_PACKAGE(Qt5 COMPONENTS Core REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Widgets REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Quick REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS QuickWidgets REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS OpenGL REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Gui REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Svg REQUIRED)
FIND_PACKAGE(Qt5Gui COMPONENTS Private REQUIRED)
FIND_PACKAGE(Qt5Svg COMPONENTS Private REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(Boost 1.70 REQUIRED)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
set(CMAKE_CXX_FLAGS "-O2")

FILE(GLOB SHAPES_CPP "src/shape/*.cpp")
FILE(GLOB CONTROLS_CPP "src/canvas/controls/*.cpp")
FILE(GLOB WIDGETS_CPP2 "src/widgets/*.cpp")

SET(SOURCES
        src/layer.cpp
        src/document.cpp
        src/undo.cpp
        ${CONTROLS_CPP}
        ${SHAPES_CPP}
        ${WIDGETS_CPP2}
        src/canvas/cache-stack.cpp
        src/window/main.cpp
        src/window/mainwindow.cpp
        src/canvas/canvas.cpp
        src/parser/svgpp-parser.cpp
        src/parser/svgpp-impl.cpp
        src/parser/svgpp-external-impl.cpp
        src/gcode/toolpath-exporter.cpp
        src/window/osxwindow.mm
        src/parser/css/svg-style-selector.h)

INCLUDE_DIRECTORIES(${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${Qt5Svg_PRIVATE_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIRS}
        third_party
        src
        /usr/local/opt/libxml2/include)


link_libraries(${LIBXML2_LIBRARY}
        ${BOOST_LIBRARIES}
        "-framework AppKit")

qt5_add_resources(SOURCES qml.qrc)

add_executable(beambird ${SOURCES})
target_link_libraries(beambird Qt::Core)
target_link_libraries(beambird Qt::Widgets)
target_link_libraries(beambird Qt::Quick)
target_link_libraries(beambird Qt::QuickWidgets)
target_link_libraries(beambird Qt::OpenGL)
target_link_libraries(beambird Qt::Gui)
target_link_libraries(beambird Qt::Svg)
