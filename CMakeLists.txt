cmake_minimum_required(VERSION 3.15)
project(vecty)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set (LIBXML2_INCLUDE_DIR "C:/Dev/LibXml2/include/libxml2")
set (LIBXML2_LIBRARY "C:/Dev/LibXml2/lib/libxml2.dll.a")
set (BOOST_ROOT "C:/Dev/boost/boost_1_70_0")
set (BOOST_INCLUDEDIR "C:/Dev/boost/boost_1_70_0/boost")
set (BOOST_LIBRARYDIR "C:/Dev/boost/boost_1_70_0/lib64-msvc-14.2")
set (OpenCV_DIR "C:/Dev/opencv/build/x64/vc14/lib")

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
FIND_PACKAGE(OpenCV REQUIRED)
FIND_PACKAGE(Boost 1.70 REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)

FIND_PACKAGE(Potrace REQUIRED)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
#SET(CMAKE_CXX_FLAGS "-O2")

GET_FILENAME_COMPONENT(SRC_DIR "src" REALPATH)

FILE(GLOB SHAPES_CPP "${SRC_DIR}/shape/*.cpp")
FILE(GLOB CONTROLS_CPP "${SRC_DIR}/canvas/controls/*.cpp")
FILE(GLOB WIDGETS_PANELS_CPP "${SRC_DIR}/widgets/panels/*.cpp")
FILE(GLOB WIDGETS_COMPONENTS_CPP "${SRC_DIR}/widgets/components/*.cpp")
FILE(GLOB WIDGETS_COMPONENTS_GRAPHICSITEMS_CPP "${SRC_DIR}/widgets/components/graphicitems/*.cpp")
FILE(GLOB SETTINGS_CPP "${SRC_DIR}/settings/*.cpp")
FILE(GLOB CONNECTION_CPP "${SRC_DIR}/connection/*.cpp")
FILE(GLOB WINDOWS_CPP "${SRC_DIR}/windows/*.cpp")
FILE(GLOB QXPOTRACE_CPP "third_party/QxPotrace/src/qxpotrace.cpp")
FILE(GLOB SERIALPORT_CPP "third_party/SerialPort/SerialPort.cpp")
FILE(GLOB CLIPPER_CPP "third_party/clipper/clipper.cpp")
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/classicUI.ini DESTINATION ${CMAKE_BINARY_DIR})
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/essentialUI.ini DESTINATION ${CMAKE_BINARY_DIR})

SET(SOURCES
        ${SRC_DIR}/layer.cpp
        ${SRC_DIR}/document.cpp
        ${SRC_DIR}/command.cpp
        ${SRC_DIR}/clipboard.cpp
        ${CONNECTION_CPP}
        ${CONTROLS_CPP}
        ${WIDGETS_PANELS_CPP}
        ${WIDGETS_COMPONENTS_CPP}
        ${WIDGETS_COMPONENTS_GRAPHICSITEMS_CPP}
        ${SHAPES_CPP}
        ${SETTINGS_CPP}
        ${WINDOWS_CPP}
        ${QXPOTRACE_CPP}
        ${SERIALPORT_CPP}
        ${CLIPPER_CPP}
        ${SRC_DIR}/canvas/cache-stack.cpp
        ${SRC_DIR}/canvas/canvas.cpp
        ${SRC_DIR}/parser/svgpp-parser.cpp
        ${SRC_DIR}/parser/svgpp-impl.cpp
        ${SRC_DIR}/parser/svgpp-external-impl.cpp
        ${SRC_DIR}/gcode/toolpath-exporter.cpp
        )

INCLUDE_DIRECTORIES(
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${Qt5Svg_PRIVATE_INCLUDE_DIRS}
        ${Qt5Widgets_PRIVATE_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIR}
        ${OpenCV_INCLUDE_DIRS}
        third_party
        ${SRC_DIR}
        #/usr/local/opt/libxml2/include
        ${POTRACE_INCLUDE_DIRS}
)

LINK_LIBRARIES(
        ${LIBXML2_LIBRARY}
        ${BOOST_LIBRARIES}
        ${OpenCV_LIBRARIES}
        ${POTRACE_LIBRARIES}
        ${POTRACE_LIBRARY}
)

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    LINK_LIBRARIES("-framework AppKit")

    SET(SOURCES
            ${SOURCES}
            ${SRC_DIR}/windows/osxwindow.mm)
ENDIF ()

# Build third_party libraries
#ADD_SUBDIRECTORY(third_party/clipper)

SET(MAIN_SOURCE ${SOURCES})
qt5_add_resources(MAIN_SOURCE "qml.qrc")

set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/images/icon.rc")
set(MACOSX_BUNDLE_ICON_FILE images/icon.icns)
set(APP_ICON_MACOSX ${CMAKE_CURRENT_SOURCE_DIR}/images/icon.icns)
set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")


add_executable(swiftray ${SRC_DIR}/main.cpp ${MAIN_SOURCE} ${APP_ICON_WINDOWS} ${APP_ICON_MACOSX})
target_link_libraries(swiftray Qt::Core)
target_link_libraries(swiftray Qt::Quick)
target_link_libraries(swiftray Qt::QuickWidgets)
target_link_libraries(swiftray Qt::OpenGL)
target_link_libraries(swiftray Qt::Gui)
target_link_libraries(swiftray Qt::Svg)
target_link_libraries(swiftray Qt::Widgets)
target_link_libraries(swiftray Qt::SerialPort)

#target_link_libraries(swiftray CLIPPER)
#target_link_libraries(swiftray Boost::thread)

target_link_libraries(swiftray LibXml2::LibXml2)

#add_subdirectory(tests)
