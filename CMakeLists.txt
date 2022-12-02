cmake_minimum_required(VERSION 3.15)
project(Swiftray VERSION 1.1.1)
message("System Name: ${CMAKE_SYSTEM_NAME}")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_BUILD 1)
set(VERSION_BETA  0) #x (e.g. 0,1,2,...)
set(VERSION_SUFFIX "") # e.g. empty("") or "-beta.x"
set(VER_PRODUCTVERSION_STR "1.1.1") # e.g. 1.x.x or 1.x.x-beta.x

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
#set(CMAKE_CXX_FLAGS "-O2")

add_definitions(-DQT_NO_KEYWORDS)
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
endif()


message(STATUS
  "CMAKE_TOOLCHAIN_FILE:"
  ${CMAKE_TOOLCHAIN_FILE}
)

include_directories(
  third_party
  ${SRC_DIR}
)

# Conan
include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_basic_setup()

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
    find_package(sentry CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} 
      sentry::sentry
    )
    add_custom_command(
        TARGET ${PROJECT_NAME}  POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools//sentry-native/crashpad_handler.exe
                ${CMAKE_BINARY_DIR}/bin/crashpad_handler.exe)
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
find_package(OpenCV REQUIRED)
include_directories(
  ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
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


# Dependency Packages handled by Conan package manager
# Boost, LibXml2, libiconv
target_link_libraries(${PROJECT_NAME} 
  ${CONAN_LIBS_BOOST}
  ${CONAN_LIBS_LIBICONV}
  ${CONAN_LIBS_LIBXML2}
  ${CONAN_LIBS_ZLIB}
)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CONAN_INCLUDE_DIRES}
)


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

