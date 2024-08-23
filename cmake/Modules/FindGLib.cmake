# FindGLib.cmake

if(WIN32)
  find_path(GLib_INCLUDE_DIR 
    NAMES glib.h 
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/glib-2.0
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_path(GLibConfig_INCLUDE_DIR
    NAMES glibconfig.h
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/glib-2.0/include
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(GLib_LIBRARY
    NAMES glib-2.0
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(GObject_LIBRARY
    NAMES gobject-2.0
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
elseif(APPLE)
  find_path(GLib_INCLUDE_DIR
    NAMES glib.h
    PATHS /usr/local/opt/glib/include/glib-2.0 /opt/homebrew/opt/glib/include/glib-2.0
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_path(GLibConfig_INCLUDE_DIR
    NAMES glibconfig.h
    PATHS /usr/local/opt/glib/lib/glib-2.0/include /opt/homebrew/opt/glib/lib/glib-2.0/include
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(GLib_LIBRARY
    NAMES glib-2.0
    PATHS /usr/local/opt/glib/lib /opt/homebrew/opt/glib/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(GObject_LIBRARY
    NAMES gobject-2.0
    PATHS /usr/local/opt/glib/lib /opt/homebrew/opt/glib/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
else()
  # Linux
  find_path(GLib_INCLUDE_DIR
    NAMES glib.h
    PATH_SUFFIXES glib-2.0
    REQUIRED
  )
  
  find_path(GLibConfig_INCLUDE_DIR
    NAMES glibconfig.h
    PATH_SUFFIXES glib-2.0/include
    REQUIRED
  )
  
  find_library(GLib_LIBRARY
    NAMES glib-2.0
    REQUIRED
  )
  
  find_library(GObject_LIBRARY
    NAMES gobject-2.0
    REQUIRED
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLib
  REQUIRED_VARS GLib_LIBRARY GObject_LIBRARY GLib_INCLUDE_DIR GLibConfig_INCLUDE_DIR
)

if(GLib_FOUND AND NOT TARGET GLib::GLib)
  add_library(GLib::GLib UNKNOWN IMPORTED)
  set_target_properties(GLib::GLib PROPERTIES
    IMPORTED_LOCATION "${GLib_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLib_INCLUDE_DIR};${GLibConfig_INCLUDE_DIR}"
  )
  
  add_library(GLib::GObject UNKNOWN IMPORTED)
  set_target_properties(GLib::GObject PROPERTIES
    IMPORTED_LOCATION "${GObject_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLib_INCLUDE_DIR};${GLibConfig_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(GLib_INCLUDE_DIR GLibConfig_INCLUDE_DIR GLib_LIBRARY GObject_LIBRARY)