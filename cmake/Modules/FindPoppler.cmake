# FindPoppler.cmake

if(WIN32)
  find_path(Poppler_INCLUDE_DIR 
    NAMES glib/poppler.h
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/poppler
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(Poppler_LIBRARY
    NAMES poppler-glib
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
elseif(APPLE)
  find_path(Poppler_INCLUDE_DIR
    NAMES glib/poppler.h
    PATHS /usr/local/opt/poppler/include/poppler /opt/homebrew/opt/poppler/include/poppler
    NO_DEFAULT_PATH
    REQUIRED
  )
  
  find_library(Poppler_LIBRARY
    NAMES poppler-glib
    PATHS /usr/local/opt/poppler/lib /opt/homebrew/opt/poppler/lib
    NO_DEFAULT_PATH
    REQUIRED
  )
else()
  # Linux
  find_path(Poppler_INCLUDE_DIR
    NAMES glib/poppler.h
    PATH_SUFFIXES poppler
    REQUIRED
  )
  
  find_library(Poppler_LIBRARY
    NAMES poppler-glib
    REQUIRED
  )
endif()

# Find Poppler version
if(Poppler_INCLUDE_DIR)
  file(STRINGS "${Poppler_INCLUDE_DIR}/cpp/poppler-version.h" POPPLER_VERSION_MAJOR_LINE REGEX "^#define POPPLER_VERSION_MAJOR +[0-9]+$")
  file(STRINGS "${Poppler_INCLUDE_DIR}/cpp/poppler-version.h" POPPLER_VERSION_MINOR_LINE REGEX "^#define POPPLER_VERSION_MINOR +[0-9]+$")
  file(STRINGS "${Poppler_INCLUDE_DIR}/cpp/poppler-version.h" POPPLER_VERSION_MICRO_LINE REGEX "^#define POPPLER_VERSION_MICRO +[0-9]+$")
  
  string(REGEX REPLACE "^#define POPPLER_VERSION_MAJOR +([0-9]+)$" "\\1" POPPLER_VERSION_MAJOR "${POPPLER_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define POPPLER_VERSION_MINOR +([0-9]+)$" "\\1" POPPLER_VERSION_MINOR "${POPPLER_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define POPPLER_VERSION_MICRO +([0-9]+)$" "\\1" POPPLER_VERSION_MICRO "${POPPLER_VERSION_MICRO_LINE}")
  
  set(Poppler_VERSION "${POPPLER_VERSION_MAJOR}.${POPPLER_VERSION_MINOR}.${POPPLER_VERSION_MICRO}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Poppler
  REQUIRED_VARS Poppler_LIBRARY Poppler_INCLUDE_DIR
  VERSION_VAR Poppler_VERSION
)

if(Poppler_FOUND AND NOT TARGET Poppler::Poppler)
  add_library(Poppler::Poppler UNKNOWN IMPORTED)
  set_target_properties(Poppler::Poppler PROPERTIES
    IMPORTED_LOCATION "${Poppler_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Poppler_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(Poppler_INCLUDE_DIR Poppler_LIBRARY)