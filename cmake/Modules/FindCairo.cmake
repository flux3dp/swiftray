# FindCairo.cmake

if(WIN32)
  find_path(Cairo_INCLUDE_DIR 
    NAMES cairo.h 
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/cairo
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(Cairo_LIBRARY
    NAMES cairo
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
elseif(APPLE)
  find_path(Cairo_INCLUDE_DIR
    NAMES cairo.h
    PATHS /usr/local/opt/cairo/include/cairo /opt/homebrew/opt/cairo/include/cairo
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(Cairo_LIBRARY
    NAMES cairo
    PATHS /usr/local/opt/cairo/lib /opt/homebrew/opt/cairo/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
else()
  # Linux
  find_path(Cairo_INCLUDE_DIR
    NAMES cairo.h
    PATH_SUFFIXES cairo
    REQUIRED
  )
  
  find_library(Cairo_LIBRARY
    NAMES cairo
    REQUIRED
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cairo
  REQUIRED_VARS Cairo_LIBRARY Cairo_INCLUDE_DIR
)

if(Cairo_FOUND AND NOT TARGET Cairo::Cairo)
  add_library(Cairo::Cairo UNKNOWN IMPORTED)
  set_target_properties(Cairo::Cairo PROPERTIES
    IMPORTED_LOCATION "${Cairo_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Cairo_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(Cairo_INCLUDE_DIR Cairo_LIBRARY)