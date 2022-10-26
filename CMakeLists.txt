cmake_minimum_required(VERSION 3.15)
project(Swiftray VERSION 1.1.0)
message("System Name: ${CMAKE_SYSTEM_NAME}")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_BUILD 0)
set(VERSION_BETA  2) #x (e.g. 0,1,2,...)
set(VERSION_SUFFIX "-beta.2") # e.g. empty("") or "-beta.x"
set(VER_PRODUCTVERSION_STR "1.1.0-beta.2") # e.g. 1.0.0 or 1.0.0-beta.x

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
#set(CMAKE_CXX_FLAGS "-O2")

# Add additional FindXXX.cmake for the project
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#set(CMAKE_FIND_DEBUG_MODE 1)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_compile_definitions(WIN32_LEAN_AND_MEAN)
  if (MSVC)
    add_compile_options(/bigobj)
  endif()
endif()
if (MINGW)
  link_libraries(ws2_32)
  message(STATUS "Link ws2_32 for resolving __imp_WSAStartup and __imp_WSACleanup undefined issue")
endif()

get_filename_component(SRC_DIR "src" REALPATH)

if (DEFINED ENABLE_SENTRY)
  message(STATUS "Sentry enabled")
  add_definitions(-DENABLE_SENTRY=${ENABLE_SENTRY})
  include_directories(
    third_party/sentry-native/install/include
  )
endif()


include_directories(
  third_party
  ${SRC_DIR}
)


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


# Set Bundle properties and add executable target
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/win.rc")
  add_executable(${PROJECT_NAME} 
    WIN32 # Make it a GUI app instead of console app
    ${SRC_DIR}/main.cpp 
    ${APP_ICON_WINDOWS})
  #set_target_properties(${PROJECT_NAME} PROPERTIES
  #  WIN32 TRUE # Make it a GUI app instead of console app
  #)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_MACOSX_RPATH 1)
  #set(APP_HELPER_FILES
  #  ${CMAKE_CURRENT_SOURCE_DIR}/xxxxx/<helper tools>
  #)
  if (DEFINED ENABLE_SENTRY)
  set(APP_FRAMEWORK_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sentry-native/install/lib/libsentry.dylib
    )
  endif()
  set(APP_RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon.icns
  )
  if (DEFINED ENABLE_SENTRY)
  set(APP_ADDITIONAL_EXEC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sentry-native/install/bin/crashpad_handler
  )
  endif()
  #set_source_files_properties(${APP_HELPER_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Helpers")
  set_source_files_properties(${APP_ADDITIONAL_EXEC_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS")
  set_source_files_properties(${APP_FRAMEWORK_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Frameworks")
  set_source_files_properties(${APP_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  add_executable(${PROJECT_NAME} 
    ${SRC_DIR}/main.cpp 
    #${APP_HELPER_FILES}
    ${APP_ADDITIONAL_EXEC_FILES}
    ${APP_FRAMEWORK_FILES}
    ${APP_RESOURCE_FILES})
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE # Create macOS app bundle
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME}
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
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_BETA=${VERSION_BETA})
target_compile_definitions(${PROJECT_NAME} PUBLIC VERSION_SUFFIX=\"${VERSION_SUFFIX}\")
target_compile_definitions(${PROJECT_NAME} PUBLIC VER_PRODUCTVERSION_STR=\"${VER_PRODUCTVERSION_STR}\\0\")

# ====================== Subdirectory sources =======================
target_sources(Swiftray PRIVATE
  ${SRC_DIR}/globals.cpp
  ${SRC_DIR}/layer.cpp
  ${SRC_DIR}/document.cpp
  ${SRC_DIR}/command.cpp
  ${SRC_DIR}/clipboard.cpp
  "qml.qrc" # replaced qt5_add_resources() when CMAKE_AUTORCC is ON
)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(Swiftray "-framework AppKit")
  target_sources(Swiftray PRIVATE
    ${SRC_DIR}/windows/osxwindow.mm
  )
endif ()

add_subdirectory(src/canvas)
add_subdirectory(src/common)
add_subdirectory(src/windows)
add_subdirectory(src/widgets)
add_subdirectory(src/settings)
add_subdirectory(src/shape)
add_subdirectory(src/parser)
add_subdirectory(src/toolpath_exporter)
add_subdirectory(src/connection)
add_subdirectory(src/machine)
add_subdirectory(src/periph)
add_subdirectory(src/executor)

# ====================== Subdirectory libraries =======================
add_subdirectory(third_party/clipper)
add_subdirectory(third_party/QxPotrace)
add_subdirectory(third_party/libdxfrw)
target_link_libraries(${PROJECT_NAME} clipper)
target_link_libraries(${PROJECT_NAME} QxPotrace)
target_link_libraries(${PROJECT_NAME} libdxfrw)

# Sentry
if (DEFINED ENABLE_SENTRY)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME}
      ${CMAKE_SOURCE_DIR}/third_party/sentry-native/install/lib/sentry.lib
    )
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(${PROJECT_NAME} 
      ${CMAKE_SOURCE_DIR}/third_party/sentry-native/install/lib/libsentry.dylib
    )
  else()
    # TODO: Linux
  endif()
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
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_compile_definitions(WIN32_LEAN_AND_MEAN)
  if(MSVC)
    add_compile_options(/bigobj)
    set(Boost_NO_WARN_NEW_VERSIONS 1)
    set(BOOST_ROOT "C:/Dev/boost_1_78_0_msvc14_2")
    set(BOOST_INCLUDEDIR "C:/Dev/boost_1_78_0_msvc14_2/boost")
    set(BOOST_LIBRARYDIR "C:/Dev/boost_1_78_0_msvc14_2/lib64-msvc-14.2")
  else()
    set(BOOST_ROOT "C:/Dev/boost_1_78_0")
    set(BOOST_INCLUDEDIR "C:/Dev/boost_1_78_0/include/boost-1_78/boost")
    set(BOOST_LIBRARYDIR "C:/Dev/boost_1_78_0/lib")
  endif()
endif()
find_package(Boost 1.70 REQUIRED COMPONENTS thread system)
if(Boost_FOUND)
  message(STATUS "Found Boost")
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES}) # link the boost component libraries
  target_link_libraries(${PROJECT_NAME} Boost::thread Boost::system) # link the boost component libraries
  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(MSVC)
      # Fix incorrect linking for msvc with boost
      target_link_libraries(${PROJECT_NAME} Boost::disable_autolinking)
    endif()
  endif()
else()
  message(FATAL_ERROR "Couldn't find boost")
endif()

# LibXml2
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if(MSVC) 
    set(ICONV_INCLUDE_DIR "C:/Dev/libraries/libconv/include")
    set(LIBXML2_LIBRARY "C:/Dev/libraries/libxml2/lib/libxml2.dll.a")
    set(LIBXML2_INCLUDE_DIR "C:/Dev/libraries/libxml2/include/libxml2")
  else()
    set(LIBXML2_INCLUDE_DIR "C:/Dev/LibXml2/include/libxml2")
    set(LIBXML2_LIBRARY "C:/Dev/LibXml2/lib/libxml2.dll.a")
  endif()
endif()
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
else()
  message(FATAL_ERROR "Couldn't find LibXml2")
endif()

# OpenCV
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if(MSVC) 
    set(OpenCV_DIR "C:/Dev/opencv_454_msvc/build")
  else()
    set(OpenCV_DIR "C:/Dev/opencv_452_mingw")
  endif()
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
#if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#  install(TARGETS ${PROJECT_NAME}
#    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install
#  )
#elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#  install(TARGETS ${PROJECT_NAME}
#    BUNDLE DESTINATION ${CMAKE_BINARY_DIR}/install
#    )
#else()
#  message(STATUS "Platform: Unknown")
#endif()


# ====================== bundle =======================
# TODO: BundleUtilities and CPack

