cmake_minimum_required(VERSION 3.20)

FIND_PACKAGE(Qt5 COMPONENTS Core REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Widgets REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Quick REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS QuickWidgets REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS OpenGL REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(Boost 1.70 REQUIRED)
FIND_PACKAGE(AppKit)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

FILE(GLOB SHAPES_CPP "src/shape/*.cpp")
FILE(GLOB CONTROLS_CPP "src/canvas/controls/*.cpp")

SET(SOURCES
    src/canvas/layer.cpp
    src/canvas/scene.cpp
    ${CONTROLS_CPP}
    ${SHAPES_CPP}
    src/widgets/layer_widget.cpp
    src/widgets/canvas_text_edit.cpp
    src/widgets/transform_widget.cpp
    src/window/main.cpp
    src/window/mainwindow.cpp
    src/canvas/vcanvas.cpp
    src/parser/svgpp_parser.cpp
    src/parser/svgpp_impl.cpp
    src/parser/svgpp_external_impl.cpp
    src/gcode/toolpath_exporter.cpp
    src/window/osxwindow.mm
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIRS}
                    third_party
                    src
                    /usr/local/opt/libxml2/include)



link_libraries(${LIBXML2_LIBRARY}
               ${BOOST_LIBRARIES}
               "-framework AppKit")

qt5_add_resources(SOURCES qml.qrc)

add_executable(vecty_bin ${SOURCES})
qt5_use_modules(vecty_bin Core Widgets Quick QuickWidgets OpenGL)
