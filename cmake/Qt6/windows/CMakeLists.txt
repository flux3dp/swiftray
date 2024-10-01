message(STATUS "CMake Configuring for Windows")

# We use MSVC to compile the project by default

set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /MDd")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MD")
add_compile_options("/permissive-") # Qt requires MSVC /permissive- option since 6.3 release

# Set build type if none was specified
set(_default_build_type "RelWithDebInfo")  
message(STATUS "Building Type ${CMAKE_BUILD_TYPE}")

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
  ${SRC_DIR}/main.cpp 
  ${CMAKE_BINARY_DIR}/resources/win.rc)

target_sources(${PROJECT_NAME} PRIVATE 
  "qml.qrc" 
  "sparkle.qrc" 
)

# ==== Set up the canvas include directory for QML ====
set(CANVAS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas)
target_include_directories(${PROJECT_NAME} PRIVATE ${CANVAS_INCLUDE_DIR})
set(QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/debug/Qt6/qml" CACHE STRING "" FORCE)
# ==== Set up QML resources ====
set(QML_RESOURCES
    src/windows/main.qml
    src/windows/qml/WelcomeDialog.qml
    src/windows/qml/MachineSetupPage.qml
    src/windows/qml/MaintenanceDialog.qml
)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}_Module
    VERSION 1.0
    RESOURCE_PREFIX "qt/qml/"
    QML_FILES
        ${QML_RESOURCES}
    IMPORTS
        Swiftray
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
  Qt::Qml
  opencv_core
  opencv_imgproc
  opencv_flann
  third_party_libs
  ${WINSPARKLE_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${WINSPARKLE_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIRS}
  ${ICONV_INCLUDE_DIR}
)

if (WINSPARKLE_FOUND)
  list (APPEND THIRD_PARTY_DLLS "${WINSPARKLE_DLL_DIR}/${WINSPARKLE_DLL}")
endif()

# ==== Add LCS library DLLS ====
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LCS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/liblcs/lib/x64/release")
else()
    set(LCS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/liblcs/lib/x86/release")
endif()

file(GLOB LCS_DLLS "${LCS_LIB_DIR}/*.dll")
list(APPEND THIRD_PARTY_DLLS ${LCS_DLLS})

# ==== Clone third-party DLLs and executables ====
if (THIRD_PARTY_DLLS)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
    ${THIRD_PARTY_DLLS}
    ${CMAKE_CURRENT_BINARY_DIR}
  )
endif(THIRD_PARTY_DLLS)

if (THIRD_PARTY_EXECS)
  add_custom_command(
    TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
    ${THIRD_PARTY_EXECS}
    ${CMAKE_CURRENT_BINARY_DIR}
  )
endif(THIRD_PARTY_EXECS)

# === Create a deploy folder and copy all necessar files into it ====

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(WINDEPLOYQT_DEBUG_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin/windeployqt.debug.bat")
    if(NOT EXISTS "${WINDEPLOYQT_DEBUG_PATH}")
        message(STATUS "windeployqt.debug.bat not found")
    else()
        message(STATUS "Using windeployqt.debug.bat")
    endif()
    set(WINDEPLOYQT_EXECUTABLE "${WINDEPLOYQT_DEBUG_PATH}")
else()
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
endif()

message(STATUS "CMAKE_INSTALL_BINDIR: ${CMAKE_INSTALL_BINDIR}")
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "WINDEPLOYQT_EXECUTABLE: ${WINDEPLOYQT_EXECUTABLE}")

file(GLOB DEFAULT_LINKED_DLLS "${CMAKE_BINARY_DIR}/*.dll")

# === Run windeployqt to add Qt plugin dlls, and copy dlls into /deploy ===
# Note: Add --compiler-runtime if using gcc to compile

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Creating deploy folder and copy dlls.."
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/deploy"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/deploy"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${DEFAULT_LINKED_DLLS} "${CMAKE_CURRENT_BINARY_DIR}/deploy"
    COMMAND "${CMAKE_COMMAND}" -E echo "Running windeployqt..."
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/qml"
        --dir "${CMAKE_CURRENT_BINARY_DIR}/deploy"
        --verbose 1
        "$<IF:$<CONFIG:Debug>,--debug,--release>"
        "${CMAKE_CURRENT_BINARY_DIR}/deploy/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMENT "Deployed Qt Dependencies"
)

# ==== Install commands ====
include(GNUInstallDirs)

# Install the main executable
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

# Install QML files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/qml/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/qml
    FILES_MATCHING PATTERN "*.qml"
)

# Install Qt Plugin Dlls
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deploy/qml
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    USE_SOURCE_PERMISSIONS
)

# Install third-party DLLs
foreach(DLL ${THIRD_PARTY_DLLS})
    install(FILES ${DLL} DESTINATION ${CMAKE_INSTALL_PREFIX})
endforeach()

# Install third-party executables
foreach(EXEC ${THIRD_PARTY_EXECS})
    install(PROGRAMS ${EXEC} DESTINATION ${CMAKE_INSTALL_PREFIX})
endforeach()

# Install deployed Qt dependencies
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deploy/
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    USE_SOURCE_PERMISSIONS
    PATTERN "${PROJECT_NAME}*" EXCLUDE  # Exclude the main executable as it's installed separately
)