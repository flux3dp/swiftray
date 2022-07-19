cmake_minimum_required(VERSION 3.15)
project(Swiftray VERSION 1.0.0)
add_compile_definitions(VERSION_MAJOR=1)
add_compile_definitions(VERSION_MINOR=0)
add_compile_definitions(VERSION_BUILD=0)
add_compile_definitions(VERSION_SUFFIX="-beta.5")


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#set(CMAKE_FIND_DEBUG_MODE 1)
if(WIN32)
  add_compile_definitions(WIN32_LEAN_AND_MEAN)
  set (LIBXML2_INCLUDE_DIR "C:/Dev/LibXml2/include/libxml2")
  set (LIBXML2_LIBRARY "C:/Dev/LibXml2/lib/libxml2.dll.a")
  set (BOOST_ROOT "C:/Dev/boost/boost_1_70_0")
  set (BOOST_INCLUDEDIR "C:/Dev/boost/boost_1_70_0/boost")
  set (BOOST_LIBRARYDIR "C:/boost_1_70_0/boost-build/lib")
  set (OpenCV_DIR "C:/Dev/opencv_452_mingw")
endif()

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
file(GLOB CONNECTION_SERIAL_CPP "${SRC_DIR}/connection/QAsyncSerial/*.cpp")
file(GLOB MOTION_CONTROLLER_JOB_CPP "${SRC_DIR}/motion_controller_job/*.cpp")
#file(GLOB SERIALPORT_CPP "third_party/SerialPort/SerialPort.cpp")
file(GLOB WINDOWS_CPP "${SRC_DIR}/windows/*.cpp")
file(GLOB PARSER_CPP "${SRC_DIR}/parser/*.cpp")
file(GLOB GRAPHICSVIEW_ITEMS_CPP "${SRC_DIR}/widgets/components/graphicitems/*.cpp")

set(SOURCES
  ${SRC_DIR}/globals.cpp
  ${SRC_DIR}/layer.cpp
  ${SRC_DIR}/document.cpp
  ${SRC_DIR}/command.cpp
  ${SRC_DIR}/clipboard.cpp
  ${CONNECTION_CPP}
  ${CONNECTION_SERIAL_CPP}
  ${MOTION_CONTROLLER_JOB_CPP}
  ${CANVAS_CPP}
  ${CONTROLS_CPP}
  ${WIDGETS_PANELS_CPP}
  ${WIDGETS_COMPONENTS_CPP}
  ${WIDGETS_COMPONENTS_GRAPHICSITEMS_CPP}
  ${SHAPES_CPP}
  ${SETTINGS_CPP}
  ${WINDOWS_CPP}
  #${SERIALPORT_CPP}
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
  ${LIBXML2_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  third_party
  ${SRC_DIR}
  /usr/local/opt/libxml2/include
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

# third_party libraries
add_subdirectory(third_party/clipper)
add_subdirectory(third_party/QxPotrace)


set(MAIN_SOURCE ${SOURCES})
qt5_add_resources(MAIN_SOURCE "qml.qrc")

# Bundle properties
if(WIN32)
  add_executable(${PROJECT_NAME} 
    ${SRC_DIR}/main.cpp 
    ${MAIN_SOURCE} 
    ${APP_ICON_WINDOWS})
  set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/win.rc")
  set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32 TRUE # Make it a GUI app instead of console app
  )
elseif(APPLE)
  set(APP_RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/images/icon.icns
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sentry/build/macOS/crashpad_handler
  )
  set_source_files_properties(${APP_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  add_executable(${PROJECT_NAME} 
    ${SRC_DIR}/main.cpp 
    ${MAIN_SOURCE}
    #${APP_ICON_MACOSX}
    ${APP_RESOURCE_FILES})
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE # Create macOS app bundle
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_INFO_STRING "Laser engraving software developed by FLUX Inc."
    MACOSX_BUNDLE_ICON_FILE icon.icns
    MACOSX_BUNDLE_GUI_IDENTIFIER com.flux.Swiftray
    MACOSX_BUNDLE_LONG_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_MINOR}${VERSION_SUFFIX}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}
    MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2022, FLUX Inc."
  )
else()
    # Linux and others
    message(STATUS "Platform: Unknown")
endif()
# Add macro definition
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_MAJOR=${VERSION_MAJOR})
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_MINOR=${VERSION_MINOR})
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_BUILD=${VERSION_BUILD})
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_SUFFIX=\"${VERSION_SUFFIX}\")


target_link_libraries(${PROJECT_NAME} Qt::Core)
target_link_libraries(${PROJECT_NAME} Qt::Quick)
target_link_libraries(${PROJECT_NAME} Qt::QuickWidgets)
target_link_libraries(${PROJECT_NAME} Qt::OpenGL)
target_link_libraries(${PROJECT_NAME} Qt::Gui)
target_link_libraries(${PROJECT_NAME} Qt::Svg)
target_link_libraries(${PROJECT_NAME} Qt::Widgets)
target_link_libraries(${PROJECT_NAME} Qt::SerialPort)

target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES})
target_link_libraries(${PROJECT_NAME} Boost::thread)
target_link_libraries(${PROJECT_NAME} Boost::system)
target_link_libraries(${PROJECT_NAME} Boost::chrono)
target_link_libraries(${PROJECT_NAME} Boost::atomic)

target_link_libraries(${PROJECT_NAME} LibXml2::LibXml2)
#target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
target_link_libraries(${PROJECT_NAME} opencv_core)
target_link_libraries(${PROJECT_NAME} opencv_imgproc)
#target_link_libraries(${PROJECT_NAME} ${POTRACE_LIBRARIES} ${POTRACE_LIBRARY})
target_link_libraries(${PROJECT_NAME} clipper)
target_link_libraries(${PROJECT_NAME} QxPotrace)
if(WIN32)

elseif(APPLE)
  target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/third_party/sentry/build/macOS/libsentry.dylib)
else()

endif()

if (MINGW)
  target_link_libraries(${PROJECT_NAME} ws2_32)
  message(STATUS "Link ws2_32 for resolving __imp_WSAStartup and __imp_WSACleanup undefined issue")
endif()

if(WIN32)
  install(TARGETS ${PROJECT_NAME} 
    CONFIGURATIONS Debug
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install/debug
  )
  install(TARGETS ${PROJECT_NAME} 
    CONFIGURATIONS Release
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install/release
  )
elseif(APPLE)
  install(TARGETS ${PROJECT_NAME} 
    CONFIGURATIONS Debug
    BUNDLE DESTINATION ${CMAKE_BINARY_DIR}/install/debug
    )
  install(TARGETS ${PROJECT_NAME} 
    CONFIGURATIONS Release
    BUNDLE DESTINATION ${CMAKE_BINARY_DIR}/install/release
    )
else()
  message(STATUS "Platform: Unknown")
endif()


# TODO: BundleUtilities and CPack

#add_subdirectory(tests)