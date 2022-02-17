cmake_minimum_required(VERSION 3.15)
project(swiftray)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#set(CMAKE_FIND_DEBUG_MODE 1)
add_compile_definitions(WIN32_LEAN_AND_MEAN)

set (LIBXML2_INCLUDE_DIR "C:/Dev/LibXml2/include/libxml2")
set (LIBXML2_LIBRARY "C:/Dev/LibXml2/lib/libxml2.dll.a")
set (BOOST_ROOT "C:/Dev/boost/boost_1_70_0")
set (BOOST_INCLUDEDIR "C:/Dev/boost/boost_1_70_0/boost")

set (BOOST_LIBRARYDIR "C:/boost_1_70_0/boost-build/lib")
#set (OpenCV_DIR "C:/Dev/opencv/build/x64/vc14/lib")
#set (OpenCV_DIR "C:/Dev/opencv/build/x64/mingw/lib")
set (OpenCV_DIR "C:/Dev/opencv-4.5.4-prebuilt/OpenCV-MinGW-Build")

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Quick REQUIRED)
find_package(Qt5 COMPONENTS QuickWidgets REQUIRED)
find_package(Qt5 COMPONENTS OpenGL REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS Svg REQUIRED)
find_package(Qt5 COMPONENTS SerialPort REQUIRED)
find_package(Qt5Gui COMPONENTS Private REQUIRED)
find_package(Qt5Widgets COMPONENTS Private REQUIRED)
find_package(Qt5Svg COMPONENTS Private REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost 1.70 REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread system chrono atomic)
find_package(Potrace REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
#set(CMAKE_CXX_FLAGS "-O2")

get_filename_component(SRC_DIR "src" REALPATH)

file(GLOB SHAPES_CPP "${SRC_DIR}/shape/*.cpp")
file(GLOB CANVAS_CPP "${SRC_DIR}/canvas/*.cpp")
file(GLOB CONTROLS_CPP "${SRC_DIR}/canvas/controls/*.cpp")
file(GLOB WIDGETS_PANELS_CPP "${SRC_DIR}/widgets/panels/*.cpp")
file(GLOB WIDGETS_COMPONENTS_CPP "${SRC_DIR}/widgets/components/*.cpp")
file(GLOB SETTINGS_CPP "${SRC_DIR}/settings/*.cpp")
file(GLOB CONNECTION_CPP "${SRC_DIR}/connection/*.cpp")
file(GLOB WINDOWS_CPP "${SRC_DIR}/windows/*.cpp")
file(GLOB QXPOTRACE_CPP "third_party/QxPotrace/src/qxpotrace.cpp")
file(GLOB SERIALPORT_CPP "third_party/SerialPort/SerialPort.cpp")
file(GLOB CLIPPER_CPP "third_party/clipper/clipper.cpp")
file(GLOB PARSER_CPP "${SRC_DIR}/parser/*.cpp")
file(GLOB GRAPHICSVIEW_ITEMS_CPP "${SRC_DIR}/widgets/components/graphicitems/*.cpp")

set(SOURCES
        ${SRC_DIR}/layer.cpp
        ${SRC_DIR}/document.cpp
        ${SRC_DIR}/command.cpp
        ${SRC_DIR}/clipboard.cpp
        ${CONNECTION_CPP}
        ${CANVAS_CPP}
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
        ${PARSER_CPP}
        ${GRAPHICSVIEW_ITEMS_CPP}
        ${SRC_DIR}/gcode/toolpath-exporter.cpp
        )

include_directories(
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

link_libraries(
        ${LIBXML2_LIBRARY}

)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    link_libraries("-framework AppKit")

    set(SOURCES
            ${SOURCES}
            ${SRC_DIR}/windows/osxwindow.mm)
endif ()

# Build third_party libraries
#ADD_SUBDIRECTORY(third_party/clipper)

set(MAIN_SOURCE ${SOURCES})
qt5_add_resources(MAIN_SOURCE "qml.qrc")

set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/images/icon.rc")
set(MACOSX_BUNDLE_ICON_FILE images/icon.icns)
set(APP_ICON_MACOSX ${CMAKE_CURRENT_SOURCE_DIR}/images/icon.icns)
set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")


add_executable(${PROJECT_NAME} ${SRC_DIR}/main.cpp ${MAIN_SOURCE} ${APP_ICON_WINDOWS} ${APP_ICON_MACOSX})
target_link_libraries(${PROJECT_NAME} Qt::Core)
target_link_libraries(${PROJECT_NAME} Qt::Quick)
target_link_libraries(${PROJECT_NAME} Qt::QuickWidgets)
target_link_libraries(${PROJECT_NAME} Qt::OpenGL)
target_link_libraries(${PROJECT_NAME} Qt::Gui)
target_link_libraries(${PROJECT_NAME} Qt::Svg)
target_link_libraries(${PROJECT_NAME} Qt::Widgets)
target_link_libraries(${PROJECT_NAME} Qt::SerialPort)

#target_link_libraries(${PROJECT_NAME} CLIPPER)
target_link_libraries(${PROJECT_NAME} 
                          ${BOOST_LIBRARIES}
                          Boost::thread
                          Boost::system
                          Boost::chrono
                          Boost::atomic)
target_link_libraries(${PROJECT_NAME} 
                          ${OpenCV_LIBRARIES})

target_link_libraries(${PROJECT_NAME} 
                          ${POTRACE_LIBRARIES}
                          ${POTRACE_LIBRARY})

target_link_libraries(${PROJECT_NAME} LibXml2::LibXml2)
if (MINGW)
    target_link_libraries(${PROJECT_NAME} ws2_32)
    message(STATUS "Link ws2_32 for resolving __imp_WSAStartup and __imp_WSACleanup undefined issue")
endif()
#add_subdirectory(tests)
