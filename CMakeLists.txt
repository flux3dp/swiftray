cmake_minimum_required(VERSION 3.15)
project(Swiftray VERSION 1.0.0)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_BUILD 0)
set(VERSION_SUFFIX "-beta.6")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
#set(CMAKE_CXX_FLAGS "-O2")

# Add additional FindXXX.cmake for the project
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#set(CMAKE_FIND_DEBUG_MODE 1)
if(WIN32)
  add_compile_definitions(WIN32_LEAN_AND_MEAN)
  if (MSVC)
    add_compile_options(/bigobj)
  endif(MSVC)
endif(WIN32)
if (MINGW)
  link_libraries(ws2_32)
  message(STATUS "Link ws2_32 for resolving __imp_WSAStartup and __imp_WSACleanup undefined issue")
endif(MINGW)

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
  third_party
  third_party/sentry-native/install/include
  ${SRC_DIR}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  link_libraries("-framework AppKit")
  set(SOURCES
    ${SOURCES}
    ${SRC_DIR}/windows/osxwindow.mm)
endif ()


# Qt
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
include_directories(
  ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
  ${Qt5Svg_PRIVATE_INCLUDE_DIRS}
  ${Qt5Widgets_PRIVATE_INCLUDE_DIRS}
)


set(MAIN_SOURCE ${SOURCES})
qt5_add_resources(MAIN_SOURCE "qml.qrc")
# Set Bundle properties and add executable target
if(WIN32)
  set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/win.rc")
  add_executable(${PROJECT_NAME} 
    WIN32 # Make it a GUI app instead of console app
    ${SRC_DIR}/main.cpp 
    ${MAIN_SOURCE} 
    ${APP_ICON_WINDOWS})
  #set_target_properties(${PROJECT_NAME} PROPERTIES
  #  WIN32 TRUE # Make it a GUI app instead of console app
  #)
elseif(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  #set(APP_HELPER_FILES
  #  ${CMAKE_CURRENT_SOURCE_DIR}/xxxxx/<helper tools>
  #)
  set(APP_FRAMEWORK_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sentry-native/install/lib/libsentry.dylib
    )
  set(APP_RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/images/icon.icns
  )
  set(APP_ADDITIONAL_EXEC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sentry-native/install/bin/crashpad_handler
  )
  #set_source_files_properties(${APP_HELPER_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Helpers")
  set_source_files_properties(${APP_ADDITIONAL_EXEC_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS")
  set_source_files_properties(${APP_FRAMEWORK_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")
  set_source_files_properties(${APP_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  add_executable(${PROJECT_NAME} 
    ${SRC_DIR}/main.cpp 
    ${MAIN_SOURCE}
    #${APP_ICON_MACOSX}
    #${APP_HELPER_FILES}
    ${APP_ADDITIONAL_EXEC_FILES}
    ${APP_FRAMEWORK_FILES}
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
# ====================== MACRO definition ======================
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_MAJOR=${VERSION_MAJOR})
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_MINOR=${VERSION_MINOR})
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_BUILD=${VERSION_BUILD})
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_SUFFIX=\"${VERSION_SUFFIX}\")


# ====================== Subdirectory libraries =======================
add_subdirectory(third_party/clipper)
add_subdirectory(third_party/QxPotrace)
target_link_libraries(${PROJECT_NAME} clipper)
target_link_libraries(${PROJECT_NAME} QxPotrace)

# Sentry
if(WIN32)
  target_link_libraries(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/third_party/sentry-native/install/lib/sentry.lib
  )
elseif(APPLE)
  target_link_libraries(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/third_party/sentry-native/install/lib/libsentry.dylib
  )
else()
  # TODO: Linux
endif()


# ====================== External libraries =======================
# Qt
target_link_libraries(${PROJECT_NAME} Qt::Core)
target_link_libraries(${PROJECT_NAME} Qt::Quick)
target_link_libraries(${PROJECT_NAME} Qt::QuickWidgets)
target_link_libraries(${PROJECT_NAME} Qt::OpenGL)
target_link_libraries(${PROJECT_NAME} Qt::Gui)
target_link_libraries(${PROJECT_NAME} Qt::Svg)
target_link_libraries(${PROJECT_NAME} Qt::Widgets)
target_link_libraries(${PROJECT_NAME} Qt::SerialPort)

# Boost
if(WIN32)
  add_compile_definitions(WIN32_LEAN_AND_MEAN)
  if(MSVC)
    add_compile_options(/bigobj)
    set(Boost_NO_WARN_NEW_VERSIONS 1)
    set(BOOST_ROOT "C:/Dev/boost_1_78_0_msvc14_2")
    set(BOOST_INCLUDEDIR "C:/Dev/boost_1_78_0_msvc14_2/boost")
    set(BOOST_LIBRARYDIR "C:/Dev/boost_1_78_0_msvc14_2/lib64-msvc-14.2")
  else(MSVC)
    set(BOOST_ROOT "C:/Dev/boost_1_78_0")
    set(BOOST_INCLUDEDIR "C:/Dev/boost_1_78_0/include/boost-1_78/boost")
    set(BOOST_LIBRARYDIR "C:/Dev/boost_1_78_0/lib")
  endif(MSVC)
endif(WIN32)
find_package(Boost 1.70 REQUIRED COMPONENTS thread system)
if(Boost_FOUND)
  message(STATUS "Found Boost")
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES}) # link the boost component libraries
  target_link_libraries(${PROJECT_NAME} Boost::thread Boost::system) # link the boost component libraries
  if(WIN32)
    if(MSVC)
      # Fix incorrect linking for msvc with boost
      target_link_libraries(${PROJECT_NAME} Boost::disable_autolinking)
    endif(MSVC)
  endif(WIN32)
else(Boost_FOUND)
  message(FATAL_ERROR "Couldn't find boost")
endif(Boost_FOUND)

# LibXml2
if(WIN32)
  if(MSVC) 
    set(ICONV_INCLUDE_DIR "C:/Dev/libraries/libconv/include")
    set(LIBXML2_LIBRARY "C:/Dev/libraries/libxml2/lib/libxml2.dll.a")
    set(LIBXML2_INCLUDE_DIR "C:/Dev/libraries/libxml2/include/libxml2")
  else(MSVC)
    set(LIBXML2_INCLUDE_DIR "C:/Dev/LibXml2/include/libxml2")
    set(LIBXML2_LIBRARY "C:/Dev/LibXml2/lib/libxml2.dll.a")
  endif(MSVC)
endif(WIN32)
find_package(LibXml2 REQUIRED)
if(LibXml2_FOUND)
  message(STATUS "Found LibXml2")
  include_directories(
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIRS}
    ${ICONV_INCLUDE_DIR}
  )
  target_link_libraries(${PROJECT_NAME} ${LIBXML2_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} LibXml2::LibXml2)
else(LibXml2_FOUND)
  message(FATAL_ERROR "Couldn't find LibXml2")
endif(LibXml2_FOUND)

# OpenCV
if(WIN32)
  if(MSVC) 
    set(OpenCV_DIR "C:/Dev/opencv_454_msvc/build")
  else(MSVC)
    set(OpenCV_DIR "C:/Dev/opencv_452_mingw")
  endif(MSVC)
endif()
find_package(OpenCV REQUIRED)
include_directories(
  ${OpenCV_INCLUDE_DIRS}
)
#target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
target_link_libraries(${PROJECT_NAME} opencv_core)
target_link_libraries(${PROJECT_NAME} opencv_imgproc)

# Potrace
find_package(Potrace REQUIRED)
include_directories(
  ${POTRACE_INCLUDE_DIRS}
)
#target_link_libraries(${PROJECT_NAME} ${POTRACE_LIBRARIES} ${POTRACE_LIBRARY})




# ====================== tests =======================
#add_subdirectory(tests)

# ====================== install =======================
#if(WIN32)
#  install(TARGETS ${PROJECT_NAME}
#    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install
#  )
#elseif(APPLE)
#  install(TARGETS ${PROJECT_NAME}
#    BUNDLE DESTINATION ${CMAKE_BINARY_DIR}/install
#    )
#else()
#  message(STATUS "Platform: Unknown")
#endif()


# ====================== bundle =======================
# TODO: BundleUtilities and CPack

