message(STATUS "CMake Configuring for Windows")

add_compile_options("/permissive-") # Qt requires MSVC /permissive- option since 6.3 release

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
  ${SRC_DIR}
  ${CMAKE_BINARY_DIR} # For cmake generated files, e.g. config.h
)

# ==== Find dependency packages ====
find_package(Qt6 COMPONENTS Core Widgets Quick QuickWidgets Websockets OpenGL QuickControls2 Qml Gui Svg SerialPort OpenGLWidgets Core5Compat REQUIRED)
find_package(Qt6Gui COMPONENTS Private REQUIRED)
find_package(Qt6Widgets COMPONENTS Private REQUIRED)
find_package(Qt6Svg COMPONENTS Private REQUIRED)
find_package(OpenCV REQUIRED)
find_package(WinSparkle REQUIRED)
find_package(sentry CONFIG REQUIRED)
find_package(Poppler REQUIRED)

# ref: https://github.com/freeorion/freeorion/blob/master/CMakeLists.txt
set_property(DIRECTORY APPEND
    PROPERTY COMPILE_DEFINITIONS
  $<$<VERSION_EQUAL:${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION},1.81>:BOOST_PHOENIX_STL_TUPLE_H_>
)

# ==== Generate source files with contents filled by cmake ====
# NOTE: This should be after all ws_find_package() function calls
configure_file(${CMAKE_SOURCE_DIR}/cmakeconfig.h.in ${CMAKE_BINARY_DIR}/config.h)
foreach( _cfg_file ${CFG_OUT_FILES} )
  configure_file( ${CMAKE_SOURCE_DIR}/${_cfg_file}.in ${CMAKE_BINARY_DIR}/${_cfg_file} @ONLY )
endforeach()

# ==== General included headers ====    
include_directories(
  ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
  ${Qt6Svg_PRIVATE_INCLUDE_DIRS}
  ${Qt6Widgets_PRIVATE_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

# ==== Set Bundle properties and add executable target ====
add_executable(${PROJECT_NAME} 
  WIN32 # Make it a GUI app instead of console app
  ${SRC_DIR}/main.cpp 
  ${CMAKE_BINARY_DIR}/resources/win.rc)

target_sources(${PROJECT_NAME} PRIVATE 
  "qml.qrc" 
  "sparkle.qrc" 
)

# ==== Subdirectory sources ====
add_subdirectory(src)
add_subdirectory(third_party)
add_library(third_party_libs INTERFACE)
target_link_libraries(third_party_libs INTERFACE
  clipper
  QxPotrace
  libdxfrw
  liblcs
  sentry::sentry)

list (APPEND THIRD_PARTY_EXECS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/sentry-native/crashpad_handler.exe)

# ==== Include and linked libraries ====
target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt::Core Qt::Quick Qt::QuickWidgets Qt::OpenGL Qt::Gui Qt::Svg Qt::Widgets Qt::WebSockets Qt::SerialPort Qt::QuickControls2 Qt6::Core5Compat Qt6::OpenGLWidgets
  opencv_core
  opencv_imgproc
  opencv_flann
  third_party_libs
  ${WINSPARKLE_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${WINSPARKLE_INCLUDE_DIRS}
  ${CONAN_INCLUDE_DIRES}
  ${ICONV_INCLUDE_DIR}
)

if (WINSPARKLE_FOUND)
  list (APPEND THIRD_PARTY_DLLS "${WINSPARKLE_DLL_DIR}/${WINSPARKLE_DLL}")
endif()


# ==== Clone third-party DLLs and executables ====
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