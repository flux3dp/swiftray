cmake_minimum_required(VERSION 3.15)
project(Swiftray VERSION 1.3.0)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 3)
set(VERSION_BUILD 0)
set(VERSION_BETA  0) #x (e.g. 0,1,2,...)
set(VERSION_SUFFIX "") # e.g. empty("") or "-beta.x"
set(PROJECT_VERSION_STR "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}${VERSION_SUFFIX}")
set(RC_VERSION ${VERSION_MAJOR},${VERSION_MINOR},${VERSION_BUILD},${VERSION_BETA})

# CXX Compiler Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
set(CMAKE_CXX_FLAGS "-O2")

add_definitions(-DQT_NO_KEYWORDS)
add_definitions(-DQT6)

# CMake Debugging Settings
# set(CMAKE_FIND_DEBUG_MODE 1)

# Add additional file assets
set(ICON_PATH "${CMAKE_SOURCE_DIR}/resources/images/")
set(CFG_OUT_FILES
  resources/win.rc
  packaging/macOS/MacOSXBundleInfo.plist
  packaging/macOS/osx-app.sh
) 

# Add additional FindXXX.cmake for the project
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/") 

message(STATUS "Project version: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}")

# Sentry configuration
if (DEFINED ENABLE_SENTRY)
  message(STATUS "Sentry enabled")
  add_definitions(-DENABLE_SENTRY=${ENABLE_SENTRY})
endif()

# Include platform-specific CMakeLists
if(APPLE)
    include(${CMAKE_SOURCE_DIR}/cmake/Qt6/unix/CMakeLists.txt)
elseif(WIN32)
    include(${CMAKE_SOURCE_DIR}/cmake/Qt6/windows/CMakeLists.txt) 
endif()