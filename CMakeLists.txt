cmake_minimum_required(VERSION 3.15)
project(vecty)

FIND_PACKAGE(Qt5 COMPONENTS Core REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Widgets REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Quick REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS QuickWidgets REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS OpenGL REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Gui REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS Svg REQUIRED)
FIND_PACKAGE(Qt5 COMPONENTS SerialPort REQUIRED)
FIND_PACKAGE(Qt5Gui COMPONENTS Private REQUIRED)
FIND_PACKAGE(Qt5Widgets COMPONENTS Private REQUIRED)
FIND_PACKAGE(Qt5Svg COMPONENTS Private REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(Boost 1.70 REQUIRED)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
#SET(CMAKE_CXX_FLAGS "-O2")

GET_FILENAME_COMPONENT(SRC_DIR "src" REALPATH)

FILE(GLOB SHAPES_CPP "${SRC_DIR}/shape/*.cpp")
FILE(GLOB CONTROLS_CPP "${SRC_DIR}/canvas/controls/*.cpp")
FILE(GLOB WIDGETS_PANELS_CPP "${SRC_DIR}/widgets/panels/*.cpp")
FILE(GLOB WIDGETS_COMPONENTS_CPP "${SRC_DIR}/widgets/components/*.cpp")
FILE(GLOB SETTINGS_CPP "${SRC_DIR}/settings/*.cpp")
FILE(GLOB CONNECTION_CPP "${SRC_DIR}/connection/*.cpp")
FILE(GLOB WINDOWS_CPP "${SRC_DIR}/windows/*.cpp")

SET(SOURCES
        ${SRC_DIR}/layer.cpp
        ${SRC_DIR}/document.cpp
        ${SRC_DIR}/command.cpp
        ${SRC_DIR}/clipboard.cpp
        ${CONNECTION_CPP}
        ${CONTROLS_CPP}
        ${WIDGETS_PANELS_CPP}
        ${WIDGETS_COMPONENTS_CPP}
        ${SHAPES_CPP}
        ${SETTINGS_CPP}
        ${WINDOWS_CPP}
        ${SRC_DIR}/canvas/cache-stack.cpp
        ${SRC_DIR}/canvas/canvas.cpp
        ${SRC_DIR}/parser/svgpp-parser.cpp
        ${SRC_DIR}/parser/svgpp-impl.cpp
        ${SRC_DIR}/parser/svgpp-external-impl.cpp
        ${SRC_DIR}/gcode/toolpath-exporter.cpp
        ${SRC_DIR}/windows/osxwindow.mm
        )

INCLUDE_DIRECTORIES(${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${Qt5Svg_PRIVATE_INCLUDE_DIRS}
        ${Qt5Widgets_PRIVATE_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIRS}
        third_party
        ${SRC_DIR}
        /usr/local/opt/libxml2/include)

link_libraries(${LIBXML2_LIBRARY}
        ${BOOST_LIBRARIES}
        "-framework AppKit")

SET(MAIN_SOURCE ${SOURCES})
qt5_add_resources(MAIN_SOURCE "qml.qrc")

add_executable(beambird ${SRC_DIR}/main.cpp ${MAIN_SOURCE})
target_link_libraries(beambird Qt::Core)
target_link_libraries(beambird Qt::Quick)
target_link_libraries(beambird Qt::QuickWidgets)
target_link_libraries(beambird Qt::OpenGL)
target_link_libraries(beambird Qt::Gui)
target_link_libraries(beambird Qt::Svg)
target_link_libraries(beambird Qt::Widgets)
target_link_libraries(beambird Qt::SerialPort)

add_subdirectory(tests)