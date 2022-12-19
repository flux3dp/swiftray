#
# Ref: https://github.com/wireshark/wireshark/blob/master/cmake/modules/FindSparkle.cmake 
# 
# Find the WinSparkle framework with vcpkg
#
# This defines the following:
#  SPARKLE_FOUND        - True if we found Sparkle
#  SPARKLE_INCLUDE_DIRS - Path to Sparkle.h, empty if not found
#  SPARKLE_LIBRARIES    - Path to Sparkle.framework, empty if not found
#  SPARKLE_VERSION      - Sparkle framework bundle version

#include(FindPackageHandleStandardArgs)

#file(GLOB USR_LOCAL_HINT "/usr/local/Sparkle-[2-9]*/")
#file(GLOB HOMEBREW_HINT "/usr/local/Caskroom/sparkle/[2-9]*/")
file(GLOB VCPKG_HINT ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})

find_path(WINSPARKLE_INCLUDE_DIR 
  winsparkle.h
  HINTS ${VCPKG_HINT}/include/winsparkle
)
find_library(WINSPARKLE_LIBRARY 
  WinSparkle
  HINTS ${USR_LOCAL_HINT}
)

if(WINSPARKLE_FOUND)
  set(WINSPARKLE_LIBRARIES ${WINSPARKLE_LIBRARY} )
  set(WINSPARKLE_INCLUDE_DIRS ${WINSPARKLE_INCLUDE_DIR} )
  message(STATUS "Found WinSparkle")
else(WINSPARKLE_FOUND)
  set(WINSPARKLE_LIBRARIES )
  set(WINSPARKLE_INCLUDE_DIRS )
endif(WINSPARKLE_FOUND)

mark_as_advanced(WINSPARKLE_INCLUDE_DIR WINSPARKLE_LIBRARY)
