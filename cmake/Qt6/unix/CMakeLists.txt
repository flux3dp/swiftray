set(MIN_MACOS_VERSION 10.14)

# QT Base Settings
set(QT_ROOT_DIR "/Users/simon/Qt/6.7.2/macos")
set(CMAKE_PREFIX_PATH "${QT_ROOT_DIR}" CACHE PATH "Path to Qt installation")

# Set build type if none was specified
set(_default_build_type "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "${_default_build_type}" CACHE STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Build type is ignored by multi-config generators.
if (NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Using \"${CMAKE_GENERATOR}\" generator and build type \"${CMAKE_BUILD_TYPE}\"")
else()
	message(STATUS "Using \"${CMAKE_GENERATOR}\" generator (multi-config)")
endif()

# Setup include path
get_filename_component(SRC_DIR "src" REALPATH)
include_directories(
  third_party
  third_party/sentry-native/install/include
  ${SRC_DIR}
	${CMAKE_BINARY_DIR} # For cmake generated files, e.g. config.h
)

# ================================= Find dependency packages =================================
# Qt6
find_package(Qt6 COMPONENTS Core Widgets Quick QuickWidgets Websockets OpenGL QuickControls2 Qml Gui Svg SerialPort OpenGLWidgets Core5Compat REQUIRED)
find_package(Qt6Gui COMPONENTS Private REQUIRED)
find_package(Qt6Widgets COMPONENTS Private REQUIRED)
find_package(Qt6Svg COMPONENTS Private REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread system)
find_package(LibXml2 REQUIRED)
find_package(Sparkle REQUIRED)
find_package(Cairo REQUIRED)
find_package(GLib REQUIRED)
find_package(Poppler REQUIRED)

# ref: https://github.com/freeorion/freeorion/blob/master/CMakeLists.txt
set_property(DIRECTORY APPEND
    PROPERTY COMPILE_DEFINITIONS
  $<$<VERSION_EQUAL:${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION},1.81>:BOOST_PHOENIX_STL_TUPLE_H_>
)

# ========================= Set variables or configs after find_package() ====================================

find_program(DMGBUILD_EXECUTABLE dmgbuild
  DOC "Path to the dmgbuild utility"
)

# https://doc-snapshots.qt.io/qt6-dev/supported-platforms.html
if(CMAKE_OSX_DEPLOYMENT_TARGET AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS MIN_MACOS_VERSION)
  message(FATAL_ERROR "Qt version ${Qt6Widgets_VERSION} requires CMAKE_OSX_DEPLOYMENT_TARGET (${CMAKE_OSX_DEPLOYMENT_TARGET}) >= ${MIN_MACOS_VERSION}")
endif()

# ================= Generate source files with contents filled by cmake ====================
# NOTE: This should be after all ws_find_package() function calls
configure_file(${CMAKE_SOURCE_DIR}/cmakeconfig.h.in ${CMAKE_BINARY_DIR}/config.h)
foreach( _cfg_file ${CFG_OUT_FILES} )
	configure_file( ${CMAKE_SOURCE_DIR}/${_cfg_file}.in ${CMAKE_BINARY_DIR}/${_cfg_file} @ONLY )
endforeach()

# ================ General included headers ===================    
include_directories(
  ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
  ${Qt6Svg_PRIVATE_INCLUDE_DIRS}
  ${Qt6Widgets_PRIVATE_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

# ===================== Set Bundle properties and add executable target ======================
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  if (DEFINED ENABLE_SENTRY)
    set(APP_FRAMEWORK_FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sentry-native/install/lib/libsentry.dylib
      ${SPARKLE_LIBRARIES}
      )
  endif()
  set(APP_RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon.icns
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/pem/dsa_pub.pem
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
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/packaging/macOS/MacOSXBundleInfo.plist
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_ICON_FILE icon.icns
    MACOSX_BUNDLE_GUI_IDENTIFIER com.flux.Swiftray
    MACOSX_BUNDLE_LONG_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}${VERSION_SUFFIX}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}
    MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}
    MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2023, FLUX Inc."
  )
  #add_custom_command(
  #  TARGET ${PROJECT_NAME} POST_BUILD 
  #  COMMAND install_name_tool -add_rpath "@executable_path/../Frameworks"
  #          $<TARGET_FILE:${PROJECT_NAME}>
  #)
else()
    # Linux and others
    message(STATUS "Platform: Unknown")
endif()

# ====================== Subdirectory sources =======================

target_sources(${PROJECT_NAME} PRIVATE 
  "qml.qrc" 
  "sparkle.qrc" 
)

add_subdirectory(src)
add_subdirectory(third_party)

add_library(third_party_libs INTERFACE)
target_link_libraries(third_party_libs INTERFACE
  clipper
  QxPotrace
  libdxfrw
  liblcs
  ${CMAKE_SOURCE_DIR}/third_party/sentry-native/install/lib/libsentry.dylib
)

# ==== Link and include libraries ====
target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CAIRO_INCLUDE_DIRS}
  ${GLib_INCLUDE_DIR}
  {Poppler_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIRS}
  ${ICONV_INCLUDE_DIR}
  ${SPARKLE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  "-framework AppKit"
  Qt::Core Qt::Quick Qt::QuickWidgets Qt::OpenGL Qt::Gui Qt::Svg Qt::Widgets Qt::WebSockets Qt::SerialPort Qt::QuickControls2 Qt6::Core5Compat Qt6::OpenGLWidgets
  opencv_core
  opencv_imgproc
  third_party_libs
  Cairo::Cairo
  GLib::GLib
  GLib::GObject
  Poppler::Poppler
  ${BOOST_LIBRARIES}
  Boost::thread Boost::system
  ${LIBXML2_LIBRARIES}
  LibXml2::LibXml2
  ${SPARKLE_LIBRARIES}
)

# ======== Copy external third_party dll/exec manually (some are auto copied by vcpkg) =======
if (THIRD_PARTY_DLLS)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
      ${THIRD_PARTY_DLLS}
      ${CMAKE_BINARY_DIR}/bin
  )
endif(THIRD_PARTY_DLLS)

if (THIRD_PARTY_EXECS)
  add_custom_command(
    TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
      ${THIRD_PARTY_EXECS}
      ${CMAKE_BINARY_DIR}/bin
  )
endif(THIRD_PARTY_EXECS)

# ======================= Packaging =======================
add_custom_target(swiftray_app_bundle)
set_target_properties(swiftray_app_bundle PROPERTIES FOLDER "Copy Tasks")
add_custom_command(TARGET swiftray_app_bundle
  POST_BUILD
  COMMAND "${CMAKE_BINARY_DIR}/packaging/macOS/osx-app.sh"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_dependencies(swiftray_app_bundle ${PROJECT_NAME})