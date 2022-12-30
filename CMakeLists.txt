cmake_minimum_required(VERSION 3.15)
project(Swiftray VERSION 1.1.2)
message("System Name: ${CMAKE_SYSTEM_NAME}")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_BUILD 2)
set(VERSION_BETA  0) #x (e.g. 0,1,2,...)
set(VERSION_SUFFIX "") # e.g. empty("") or "-beta.x"
#set(PROJECT_VERSION_STR "1.1.2") # e.g. 1.x.x or 1.x.x-beta.x
set(PROJECT_VERSION_STR "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}${VERSION_SUFFIX}")
# Used in .rc files and manifests
set(RC_VERSION ${VERSION_MAJOR},${VERSION_MINOR},${VERSION_BUILD},${VERSION_BETA})

message(STATUS "Swiftray Ver: ${PROJECT_VERSION_STR}, MaV: ${VERSION_MAJOR}, MiV: ${VERSION_MINOR}, BuildV: ${VERSION_BUILD}, EV: ${VERSION_SUFFIX}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
#set(CMAKE_CXX_FLAGS "-O2")

# Set a default build type if none was specified
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

# Add additional FindXXX.cmake for the project
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#set(CMAKE_FIND_DEBUG_MODE 1)


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_compile_definitions(WIN32_LEAN_AND_MEAN)
  if (MSVC)
    #add_compile_definitions(NTDDI_VERSION=NTDDI_WIN7)
    #add_compile_definitions(_WIN32_WINNT=_WIN32_WINNT_WIN7)
    add_compile_options(/bigobj)
  endif()
endif()
if (MINGW)
  link_libraries(ws2_32)
  message(STATUS "Link ws2_32 for resolving __imp_WSAStartup and __imp_WSACleanup undefined issue")
endif()

# Compiler specific
#if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  #message("CMAKE_SYSTEM_VERSION: "  ${CMAKE_SYSTEM_VERSION})
  #message("WINDOWS SDK VERSION: " ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
  #message("SYSTEM VERSION" ${CMAKE_HOST_SYSTEM_VERSION})
  #message("CMAKE_SYSTEM_NAME" ${CMAKE_SYSTEM_NAME})
  # MSVC 14.28 + C11 enables C5105, but older Windows SDKs aren't completely compatible.
	# Windows SDK 10.0.17763.0 generates syntax errors with C11 enabled.
#	if (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS 10.0.18362.0)
#		message(FATAL_ERROR "Windows SDK ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} doesn't support C17. Please make sure you're using 10.0.20348.0 or later.")
#	endif()
	# Windows SDK 10.0.18362.0 to 10.0.19041.685 generate warning C5105 with C11 enabled.
#	if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS 10.0.20348.0)
#		message(WARNING "Windows SDK ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} doesn't support C17. Please make sure you're using 10.0.20348.0 or later.")
#	endif()
#else() # ! MSVC
#endif()

add_definitions(-DQT_NO_KEYWORDS)

if (DEFINED ENABLE_SENTRY)
  message(STATUS "Sentry enabled")
  add_definitions(-DENABLE_SENTRY=${ENABLE_SENTRY})
endif()

get_filename_component(SRC_DIR "src" REALPATH)
include_directories(
  third_party
  ${SRC_DIR}
	${CMAKE_BINARY_DIR} # For cmake generated files, e.g. config.h
)

# ================= Package managers ====================
message(STATUS
  "CMAKE_TOOLCHAIN_FILE:"
  ${CMAKE_TOOLCHAIN_FILE}
)

# Conan Package Manager
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # Recommended way
  #if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  #  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  #  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
  #                "${CMAKE_BINARY_DIR}/conan.cmake"
  #                TLS_VERIFY ON)
  #endif()
  #include(${CMAKE_BINARY_DIR}/conan.cmake)
  #conan_cmake_configure(REQUIRES boost/1.70.0
  #                                libxml2/2.9.14
  #                                libiconv/1.17
  #                                libpotrace/1.16@user/testing
  #                      GENERATORS cmake_find_package
  #                      IMPORTS "bin, *.dll -> ./bin"
  #                              "lib, *.dylib* -> ./bin")
  #conan_cmake_autodetect(settings)
  #conan_cmake_install(PATH_OR_REFERENCE .
  #                    BUILD missing
  #                    REMOTE conancenter
  #                    SETTINGS ${settings})
  
  # Old way, using cmake generator
  if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  elseif(EXISTS "${CMAKE_BINARY_DIR}/conan-dependencies/conanbuildinfo.cmake") # For Qt Creator
    include("${CMAKE_BINARY_DIR}/conan-dependencies/conanbuildinfo.cmake")
  else()
    message(FATAL_ERROR "Unable to find conanbuildinfo.cmake which should be generated by conan install cmd")
  endif()
  conan_basic_setup()
endif()

# ======================================= Helper Macros ======================================
# Resets cache variables if the <PackageName>_LIBRARY has become invalid.
# Call it before a find_package(<PackageName> ...) invocation that uses
# find_library(<PackageName>_LIBRARY ...).
#
# Usage: reset_find_package(<PackageName> [<extra variables to clear>])
function(reset_find_package _package_name)
	set(variables
		# find_library / find_package
		${_package_name}_LIBRARY
		${_package_name}_INCLUDE_DIR
		# mark_as_advanced
		${_package_name}_LIBRARIES
		${_package_name}_INCLUDE_DIRS
		# Others
		${_package_name}_DLL_DIR
		${_package_name}_DLLS
		${_package_name}_DLL
		${_package_name}_PDB
		${ARGN}
	)
	if(NOT ${_package_name}_LIBRARY OR EXISTS ${${_package_name}_LIBRARY})
		# Cache variable is already missing or cache entry is valid.
		return()
	endif()
	message(STATUS "Package ${_package_name} has changed, clearing cache.")
	foreach(_var IN LISTS variables)
		unset(${_var} CACHE)
	endforeach()
endfunction()

# ws_find_package(<PackageName>
#             <CMakeOptions.txt boolean variable>
#             <cmakeconfig.h.in macro definition>
#             [remaining find_package() arguments])
macro(ws_find_package _package_name _enable_package _package_cmakedefine)
	if(${_enable_package})
		# Clear outdated cache variables if not already.
		reset_find_package(${_package_name})
		find_package(${_package_name} ${ARGN})
		if(${_package_name}_FOUND)
			set(${_package_cmakedefine} 1)
		endif()
	endif()
endmacro()


# ================================= Find dependency packages =================================
# Qt
if(USE_qt6)
  set(qtver "6")
else()
  set(qtver "5")
endif()
find_package(Qt${qtver} COMPONENTS Core REQUIRED)
find_package(Qt${qtver} COMPONENTS Widgets REQUIRED)
find_package(Qt${qtver} COMPONENTS Quick REQUIRED)
find_package(Qt${qtver} COMPONENTS QuickWidgets REQUIRED)
find_package(Qt${qtver} COMPONENTS OpenGL REQUIRED)
find_package(Qt${qtver} COMPONENTS Gui REQUIRED)
find_package(Qt${qtver} COMPONENTS Svg REQUIRED)
find_package(Qt${qtver} COMPONENTS SerialPort REQUIRED)
find_package(Qt${qtver}Gui COMPONENTS Private REQUIRED)
find_package(Qt${qtver}Widgets COMPONENTS Private REQUIRED)
find_package(Qt${qtver}Svg COMPONENTS Private REQUIRED)

find_package(OpenCV REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_package(Boost REQUIRED COMPONENTS thread system)
  find_package(LibXml2 REQUIRED)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  ws_find_package(WinSparkle ENABLE_SPARKLE HAVE_SOFTWARE_UPDATE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  ws_find_package(Sparkle ENABLE_SPARKLE HAVE_SOFTWARE_UPDATE)
else()
  # TODO: Linux
endif()

if (DEFINED ENABLE_SENTRY)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(sentry CONFIG REQUIRED)
  endif()
endif()


# ========================= Set variables or configs after find_package() ====================================
if (Qt${qtver}Widgets_VERSION VERSION_LESS 5.10)
  message(FATAL_ERROR "Qt 5.12 or later is required.")
endif()
if (Qt${qtver}Widgets_VERSION VERSION_LESS 5.12)
  message(WARNING "Wireshark can be build with this version of Qt, though 5.12 or higher is recommended.")
endif()
#if(NOT DEFINED MOC_OPTIONS)
#  # Squelch moc verbose "nothing to do" output
#  set(MOC_OPTIONS -nn)
#endif()

# CMake uses qmake to find Qt4. It relies on Qt's CMake modules
# to find Qt5. This means that we can't assume that the qmake
# in our PATH is the correct one. We can fetch qmake's location
# from Qt5::qmake, which is defined in Qt5CoreConfigExtras.cmake.
get_target_property(QT_QMAKE_EXECUTABLE Qt${qtver}::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_path "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
set(QT_BIN_PATH "${_qt_bin_path}" CACHE INTERNAL
  "Path to qmake, macdeployqt, windeployqt, and other Qt utilities."
)
# Use qmake to find windeployqt and macdeployqt. Ideally one of
# the modules in ${QTDIR}/lib/cmake would do this for us.
if(WIN32)
  if (USE_qt6 AND USE_MSYSTEM)
    set(_windeployqt_name "windeployqt-qt6")
  else()
    set(_windeployqt_name "windeployqt")
  endif()
  find_program(QT_WINDEPLOYQT_EXECUTABLE ${_windeployqt_name}
    HINTS "${QT_BIN_PATH}"
    DOC "Path to the windeployqt utility."
  )
elseif(APPLE)
  find_program(QT_MACDEPLOYQT_EXECUTABLE macdeployqt
    HINTS "${QT_BIN_PATH}"
    DOC "Path to the macdeployqt utility."
  )
  find_program(DMGBUILD_EXECUTABLE dmgbuild
    DOC "Path to the dmgbuild utility"
  )
  # https://doc.qt.io/qt-5/supported-platforms.html
  # https://doc.qt.io/qt-5.11/supported-platforms-and-configurations.html
  # https://doc.qt.io/qt-5.15/supported-platforms.html
  # https://doc-snapshots.qt.io/qt6-dev/supported-platforms.html
  if(Qt${qtver}Widgets_VERSION VERSION_GREATER_EQUAL "6.0.0")
    set(MIN_MACOS_VERSION 10.14)
  elseif(Qt5Widgets_VERSION VERSION_GREATER_EQUAL "5.14.0")
    set(MIN_MACOS_VERSION 10.13)
  elseif(Qt5Widgets_VERSION VERSION_GREATER_EQUAL "5.12.0")
    set(MIN_MACOS_VERSION 10.12)
  elseif(Qt5Widgets_VERSION VERSION_GREATER_EQUAL "5.10.0")
    set(MIN_MACOS_VERSION 10.11)
  endif()
  if(CMAKE_OSX_DEPLOYMENT_TARGET AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS MIN_MACOS_VERSION)
    message(FATAL_ERROR "Qt version ${Qt${qtver}Widgets_VERSION} requires CMAKE_OSX_DEPLOYMENT_TARGET (${CMAKE_OSX_DEPLOYMENT_TARGET}) >= ${MIN_MACOS_VERSION}")
  endif()
endif()

# Qt requires MSVC /permissive- option since 6.3 release
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND Qt${qtver}Widgets_VERSION VERSION_GREATER_EQUAL "6.3.0")
  add_compile_options("/permissive-")
endif()




# ================= Generate source files with contents filled by cmake ====================
# NOTE: This should be after all ws_find_package() function calls
set(ICON_PATH "${CMAKE_SOURCE_DIR}/resources/images/")
configure_file(${CMAKE_SOURCE_DIR}/cmakeconfig.h.in ${CMAKE_BINARY_DIR}/config.h)
set(CFG_OUT_FILES
	resources/win.rc
	packaging/macOS/MacOSXBundleInfo.plist
  packaging/macOS/osx-app.sh
)
foreach( _cfg_file ${CFG_OUT_FILES} )
	configure_file( ${CMAKE_SOURCE_DIR}/${_cfg_file}.in ${CMAKE_BINARY_DIR}/${_cfg_file} @ONLY )
endforeach()

# ================ General included headers ===================    
include_directories(
  ${Qt${qtver}Gui_PRIVATE_INCLUDE_DIRS}
  ${Qt${qtver}Svg_PRIVATE_INCLUDE_DIRS}
  ${Qt${qtver}Widgets_PRIVATE_INCLUDE_DIRS}
)

# ===================== Set Bundle properties and add executable target ======================
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS_RC_FILES ${CMAKE_BINARY_DIR}/resources/win.rc)
  add_executable(${PROJECT_NAME} 
    WIN32 # Make it a GUI app instead of console app
    ${SRC_DIR}/main.cpp 
    ${WINDOWS_RC_FILES})
  #set_target_properties(${PROJECT_NAME} PROPERTIES
  #  WIN32 TRUE # Make it a GUI app instead of console app
  #)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  #set(CMAKE_SKIP_RPATH TRUE)
  #set(CMAKE_SKIP_BUILD_RPATH TRUE)
  #set(CMAKE_SKIP_INSTALL_RPATH TRUE)
  #set(CMAKE_MACOSX_RPATH 1)
  #set(APP_HELPER_FILES
  #  ${CMAKE_CURRENT_SOURCE_DIR}/xxxxx/<helper tools>
  #)
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
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}
    MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}
    MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2022, FLUX Inc."
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
set(SWIFTRAY_SRC
  ${SRC_DIR}/globals.cpp
  ${SRC_DIR}/layer.cpp
  ${SRC_DIR}/document.cpp
  ${SRC_DIR}/command.cpp
  ${SRC_DIR}/clipboard.cpp
  ${SRC_DIR}/main_application.cpp
	)
SET(SWIFTRAY_QRC
  "qml.qrc" # replaced qt5_add_resources() when CMAKE_AUTORCC is ON
  "sparkle.qrc" # 
)
set(swiftray_FILES
  ${SWIFTRAY_SRC}
  ${SWIFTRAY_QRC}
)

target_sources(${PROJECT_NAME} PRIVATE
  ${swiftray_FILES}
)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(${PROJECT_NAME} "-framework AppKit")
  target_sources(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}/windows/osxwindow.mm
  )
endif ()

add_subdirectory(src/canvas)
add_subdirectory(src/utils)
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
    #find_package(sentry CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} 
      sentry::sentry
    )
    list (APPEND THIRD_PARTY_EXECS 
      ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools//sentry-native/crashpad_handler.exe
    )
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(
      third_party/sentry-native/install/include
    )
    target_link_libraries(${PROJECT_NAME} 
      ${CMAKE_SOURCE_DIR}/third_party/sentry-native/install/lib/libsentry.dylib
    )
  else()
    # TODO: Linux
  endif()
endif()


# ====================== External libraries =======================
# Collect all linked Libraries into a list

# Qt
target_link_libraries(${PROJECT_NAME} Qt::Core)
target_link_libraries(${PROJECT_NAME} Qt::Quick)
target_link_libraries(${PROJECT_NAME} Qt::QuickWidgets)
target_link_libraries(${PROJECT_NAME} Qt::OpenGL)
target_link_libraries(${PROJECT_NAME} Qt::Gui)
target_link_libraries(${PROJECT_NAME} Qt::Svg)
target_link_libraries(${PROJECT_NAME} Qt::Widgets)
target_link_libraries(${PROJECT_NAME} Qt::SerialPort)



# OpenCV
include_directories(
  ${OpenCV_INCLUDE_DIRS}
)
#target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
#target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} opencv_core)
target_link_libraries(${PROJECT_NAME} opencv_imgproc)

# Cairo
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  find_path(CAIRO_INCLUDE_DIR 
    NAMES cairo.h 
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/cairo
    REQUIRED)
  find_library(CAIRO_LIBRARY cairo REQUIRED)
  include_directories(${CAIRO_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME}
    ${CAIRO_LIBRARY}
  )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_include_directories(${PROJECT_NAME} PRIVATE
    /usr/local/opt/cairo/include/cairo
    #/opt/homebrew/opt/cairo/include/cairo
  )
  target_link_libraries(${PROJECT_NAME} 
    /usr/local/opt/cairo/lib/libcairo.dylib
    #/opt/homebrew/opt/cairo/lib/libcairo.dylib
  )
else()
  # TODO: Linux
endif()

# GLIB and GOBJECT
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # TODO: Pack into a FindGLIB.cmake and FindGObject.cmake
  find_path(GLIB_INCLUDE_DIR 
    NAMES glib.h 
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/glib-2.0
    REQUIRED)
  find_path(GLIBCONFIG_INCLUDE_DIR
    NAMES glibconfig.h
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/glib-2.0/include
    REQUIRED
  )
  find_library(GLIB_LIBRARY glib-2.0 REQUIRED)
  find_library(GOBJECT_LIBRARY gobject-2.0 REQUIRED)
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${GLIB_INCLUDE_DIR}
  )
  include_directories(${GLIBCONFIG_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME}
    ${GLIB_LIBRARY}
  )
  target_link_libraries(${PROJECT_NAME}
    ${GOBJECT_LIBRARY}
  )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_include_directories(${PROJECT_NAME} PRIVATE
    /usr/local/opt/glib/include/glib-2.0
    /usr/local/opt/glib/lib/glib-2.0/include
    #/opt/homebrew/opt/glib/include/glib-2.0
    #/opt/homebrew/opt/glib/lib/glib-2.0/include
  )
  target_link_libraries(${PROJECT_NAME} 
    /usr/local/opt/glib/lib/libglib-2.0.dylib
    /usr/local/opt/glib/lib/libgobject-2.0.dylib
    #/opt/homebrew/opt/glib/lib/libglib-2.0.dylib
    #/opt/homebrew/opt/glib/lib/libgobject-2.0.dylib
  )
else()
  # TODO: Linux
endif()

# Poppler
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # TODO: Pack into a FindPoppler.cmakes
  find_path(POPPLER_INCLUDE_DIR 
    NAMES glib/poppler.h
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/poppler
    REQUIRED)
  find_library(POPPLER_LIBRARY poppler-glib REQUIRED)
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${POPPLER_INCLUDE_DIR}
  )
  target_link_libraries(${PROJECT_NAME}
    ${POPPLER_LIBRARY}
  )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_include_directories(${PROJECT_NAME} PRIVATE
    /usr/local/opt/poppler/include/poppler
    #/opt/homebrew/opt/poppler/include/poppler
  )
  target_link_libraries(${PROJECT_NAME} 
    /usr/local/opt/poppler/lib/libpoppler-glib.dylib
    #/opt/homebrew/opt/poppler/lib/libpoppler-glib.dylib
  )
else()
  # TODO: Linux
endif()


# Boost, LibXml2, libiconv
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # Dependency Packages handled by Conan package manager  
  list(APPEND swiftray_EXT_LIBS 
      ${CONAN_LIBS_BOOST}
      ${CONAN_LIBS_LIBICONV}
      ${CONAN_LIBS_LIBXML2}
      ${CONAN_LIBS_ZLIB}
  )
  list(APPEND swiftray_EXT_INCLUDE_DIRS 
      ${CONAN_INCLUDE_DIRES}
  )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Boost
  if(Boost_FOUND)
    message(STATUS "Found Boost")
  else()
    message(FATAL_ERROR "Couldn't find boost")
  endif()
  # LibXml2 & LibIconv
  if(LibXml2_FOUND)
    message(STATUS "Found LibXml2")
  else()
    message(FATAL_ERROR "Couldn't find LibXml2")
  endif()
  
  list(APPEND swiftray_EXT_LIBS 
    ${BOOST_LIBRARIES}
    Boost::thread Boost::system
    ${LIBXML2_LIBRARIES}
    LibXml2::LibXml2
  )
  list(APPEND swiftray_EXT_INCLUDE_DIRS 
    ${Boost_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIRS}
    ${ICONV_INCLUDE_DIR}
  )
endif()

# Sparkle
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  list(APPEND swiftray_EXT_LIBS ${WINSPARKLE_LIBRARIES})
  list(APPEND swiftray_EXT_INCLUDE_DIRS ${WINSPARKLE_INCLUDE_DIRS})
  if (WINSPARKLE_FOUND)
    list (APPEND THIRD_PARTY_DLLS "${WINSPARKLE_DLL_DIR}/${WINSPARKLE_DLL}")
  endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND swiftray_EXT_LIBS ${SPARKLE_LIBRARIES})
  list(APPEND swiftray_EXT_INCLUDE_DIRS ${SPARKLE_INCLUDE_DIRS})
  #message("SPARKLE_LIB: " ${SPARKLE_LIBRARY})
  #add_custom_command(
  #    TARGET ${PROJECT_NAME}  POST_BUILD
  #    COMMAND ${CMAKE_COMMAND} -E copy_directory
  #            ${SPARKLE_LIBRARY}
  #            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks/Sparkle.framework
  #)
else()
  # TODO: Linux
endif()



# ===============================================================
# Link exec target to  all linked libraries
target_link_libraries(${PROJECT_NAME} 
  ${swiftray_EXT_LIBS}
)
# Add include headers of libraries to exec target
target_include_directories(${PROJECT_NAME} PRIVATE 
  ${swiftray_EXT_INCLUDE_DIRS}
)


# ======== Copy external third_party dll/exec manually (some are auto copied by vcpkg) =======
if (THIRD_PARTY_DLLS)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
      ${THIRD_PARTY_DLLS}
      ${CMAKE_BINARY_DIR}/bin
  )
  #install(FILES ${THIRD_PARTY_DLLS} DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif(THIRD_PARTY_DLLS)

if (THIRD_PARTY_EXECS)
  add_custom_command(
    TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
      ${THIRD_PARTY_EXECS}
      ${CMAKE_BINARY_DIR}/bin
  )
  #install(FILES ${THIRD_PARTY_EXECS} DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif(THIRD_PARTY_EXECS)



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
# TODO: Use BundleUtilities and CPack?

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_custom_target(swiftray_app_bundle)
  set_target_properties(swiftray_app_bundle PROPERTIES FOLDER "Copy Tasks")
  add_custom_command(TARGET swiftray_app_bundle
    POST_BUILD
    COMMAND "${CMAKE_BINARY_DIR}/packaging/macOS/osx-app.sh"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  add_dependencies(swiftray_app_bundle ${PROJECT_NAME})
endif()
