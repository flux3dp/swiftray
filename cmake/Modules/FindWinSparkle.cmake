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
  HINTS ${VCPKG_HINT}/lib
  REQUIRED
)

# handle the QUIETLY and REQUIRED arguments and set WINSPARKLE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WinSparkle DEFAULT_MSG WINSPARKLE_LIBRARY WINSPARKLE_INCLUDE_DIR)

if(WINSPARKLE_FOUND)
  set(WINSPARKLE_LIBRARIES ${WINSPARKLE_LIBRARY} )
  set(WINSPARKLE_INCLUDE_DIRS ${WINSPARKLE_INCLUDE_DIR} )
  if (WIN32)
    set (WINSPARKLE_DLL_DIR "${VCPKG_HINT}/bin"
      CACHE PATH "Path to the WinSparkle DLL"
    )
    file(GLOB _winsparkle_dll RELATIVE "${WINSPARKLE_DLL_DIR}"
      "${WINSPARKLE_DLL_DIR}/WinSparkle.dll"
    )
    set ( WINSPARKLE_DLL ${_winsparkle_dll}
      # We're storing filenames only. Should we use STRING instead?
      CACHE FILEPATH "WinSparkle DLL file name"
    )
    mark_as_advanced( WINSPARKLE_DLL_DIR WINSPARKLE_DLL )
  endif()

  message(STATUS "Found WinSparkle")
else(WINSPARKLE_FOUND)
  set(WINSPARKLE_LIBRARIES )
  set(WINSPARKLE_INCLUDE_DIRS )
  set(WINSPARKLE_DLL_DIR )
  set(WINSPARKLE_DLL )
endif(WINSPARKLE_FOUND)

mark_as_advanced(WINSPARKLE_LIBRARIES WINSPARKLE_INCLUDE_DIRS)


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


ws_find_package(WinSparkle ENABLE_SPARKLE HAVE_SOFTWARE_UPDATE)