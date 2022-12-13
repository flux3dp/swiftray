include(CMakeParseArguments)

macro(conan_find_apple_frameworks FRAMEWORKS_FOUND FRAMEWORKS SUFFIX BUILD_TYPE)
    if(APPLE)
        if(CMAKE_BUILD_TYPE)
            set(_BTYPE ${CMAKE_BUILD_TYPE})
        elseif(NOT BUILD_TYPE STREQUAL "")
            set(_BTYPE ${BUILD_TYPE})
        endif()
        if(_BTYPE)
            if(${_BTYPE} MATCHES "Debug|_DEBUG")
                set(CONAN_FRAMEWORKS${SUFFIX} ${CONAN_FRAMEWORKS${SUFFIX}_DEBUG} ${CONAN_FRAMEWORKS${SUFFIX}})
                set(CONAN_FRAMEWORK_DIRS${SUFFIX} ${CONAN_FRAMEWORK_DIRS${SUFFIX}_DEBUG} ${CONAN_FRAMEWORK_DIRS${SUFFIX}})
            elseif(${_BTYPE} MATCHES "Release|_RELEASE")
                set(CONAN_FRAMEWORKS${SUFFIX} ${CONAN_FRAMEWORKS${SUFFIX}_RELEASE} ${CONAN_FRAMEWORKS${SUFFIX}})
                set(CONAN_FRAMEWORK_DIRS${SUFFIX} ${CONAN_FRAMEWORK_DIRS${SUFFIX}_RELEASE} ${CONAN_FRAMEWORK_DIRS${SUFFIX}})
            elseif(${_BTYPE} MATCHES "RelWithDebInfo|_RELWITHDEBINFO")
                set(CONAN_FRAMEWORKS${SUFFIX} ${CONAN_FRAMEWORKS${SUFFIX}_RELWITHDEBINFO} ${CONAN_FRAMEWORKS${SUFFIX}})
                set(CONAN_FRAMEWORK_DIRS${SUFFIX} ${CONAN_FRAMEWORK_DIRS${SUFFIX}_RELWITHDEBINFO} ${CONAN_FRAMEWORK_DIRS${SUFFIX}})
            elseif(${_BTYPE} MATCHES "MinSizeRel|_MINSIZEREL")
                set(CONAN_FRAMEWORKS${SUFFIX} ${CONAN_FRAMEWORKS${SUFFIX}_MINSIZEREL} ${CONAN_FRAMEWORKS${SUFFIX}})
                set(CONAN_FRAMEWORK_DIRS${SUFFIX} ${CONAN_FRAMEWORK_DIRS${SUFFIX}_MINSIZEREL} ${CONAN_FRAMEWORK_DIRS${SUFFIX}})
            endif()
        endif()
        foreach(_FRAMEWORK ${FRAMEWORKS})
            # https://cmake.org/pipermail/cmake-developers/2017-August/030199.html
            find_library(CONAN_FRAMEWORK_${_FRAMEWORK}_FOUND NAMES ${_FRAMEWORK} PATHS ${CONAN_FRAMEWORK_DIRS${SUFFIX}} CMAKE_FIND_ROOT_PATH_BOTH)
            if(CONAN_FRAMEWORK_${_FRAMEWORK}_FOUND)
                list(APPEND ${FRAMEWORKS_FOUND} ${CONAN_FRAMEWORK_${_FRAMEWORK}_FOUND})
            else()
                message(FATAL_ERROR "Framework library ${_FRAMEWORK} not found in paths: ${CONAN_FRAMEWORK_DIRS${SUFFIX}}")
            endif()
        endforeach()
    endif()
endmacro()


#################
###  CAIRO
#################
set(CONAN_CAIRO_ROOT "C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222")
set(CONAN_INCLUDE_DIRS_CAIRO "C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/include"
			"C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/include/cairo")
set(CONAN_LIB_DIRS_CAIRO "C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/lib")
set(CONAN_BIN_DIRS_CAIRO )
set(CONAN_RES_DIRS_CAIRO )
set(CONAN_SRC_DIRS_CAIRO )
set(CONAN_BUILD_DIRS_CAIRO "C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/")
set(CONAN_FRAMEWORK_DIRS_CAIRO )
set(CONAN_LIBS_CAIRO cairo-script-interpreter cairo-gobject cairo)
set(CONAN_PKG_LIBS_CAIRO cairo-script-interpreter cairo-gobject cairo)
set(CONAN_SYSTEM_LIBS_CAIRO gdi32 msimg32 user32)
set(CONAN_FRAMEWORKS_CAIRO )
set(CONAN_FRAMEWORKS_FOUND_CAIRO "")  # Will be filled later
set(CONAN_DEFINES_CAIRO "-DCAIRO_WIN32_STATIC_BUILD=1")
set(CONAN_BUILD_MODULES_PATHS_CAIRO )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_CAIRO "CAIRO_WIN32_STATIC_BUILD=1")

set(CONAN_C_FLAGS_CAIRO "")
set(CONAN_CXX_FLAGS_CAIRO "")
set(CONAN_SHARED_LINKER_FLAGS_CAIRO "")
set(CONAN_EXE_LINKER_FLAGS_CAIRO "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_CAIRO_LIST "")
set(CONAN_CXX_FLAGS_CAIRO_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_CAIRO_LIST "")
set(CONAN_EXE_LINKER_FLAGS_CAIRO_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_CAIRO "${CONAN_FRAMEWORKS_CAIRO}" "_CAIRO" "")
# Append to aggregated values variable
set(CONAN_LIBS_CAIRO ${CONAN_PKG_LIBS_CAIRO} ${CONAN_SYSTEM_LIBS_CAIRO} ${CONAN_FRAMEWORKS_FOUND_CAIRO})


#################
###  GLIB
#################
set(CONAN_GLIB_ROOT "C:/.conan/ecd301/1")
set(CONAN_INCLUDE_DIRS_GLIB "C:/.conan/ecd301/1/include"
			"C:/.conan/ecd301/1/include/gio-win32-2.0"
			"C:/.conan/ecd301/1/include/glib-2.0"
			"C:/.conan/ecd301/1/lib/glib-2.0/include")
set(CONAN_LIB_DIRS_GLIB "C:/.conan/ecd301/1/lib")
set(CONAN_BIN_DIRS_GLIB "C:/.conan/ecd301/1/bin")
set(CONAN_RES_DIRS_GLIB )
set(CONAN_SRC_DIRS_GLIB )
set(CONAN_BUILD_DIRS_GLIB )
set(CONAN_FRAMEWORK_DIRS_GLIB )
set(CONAN_LIBS_GLIB gthread-2.0 gio-2.0 gmodule-2.0 gobject-2.0 glib-2.0)
set(CONAN_PKG_LIBS_GLIB gthread-2.0 gio-2.0 gmodule-2.0 gobject-2.0 glib-2.0)
set(CONAN_SYSTEM_LIBS_GLIB iphlpapi dnsapi shlwapi ws2_32 ole32 shell32 user32 advapi32)
set(CONAN_FRAMEWORKS_GLIB )
set(CONAN_FRAMEWORKS_FOUND_GLIB "")  # Will be filled later
set(CONAN_DEFINES_GLIB )
set(CONAN_BUILD_MODULES_PATHS_GLIB )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_GLIB )

set(CONAN_C_FLAGS_GLIB "")
set(CONAN_CXX_FLAGS_GLIB "")
set(CONAN_SHARED_LINKER_FLAGS_GLIB "")
set(CONAN_EXE_LINKER_FLAGS_GLIB "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_GLIB_LIST "")
set(CONAN_CXX_FLAGS_GLIB_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_GLIB_LIST "")
set(CONAN_EXE_LINKER_FLAGS_GLIB_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_GLIB "${CONAN_FRAMEWORKS_GLIB}" "_GLIB" "")
# Append to aggregated values variable
set(CONAN_LIBS_GLIB ${CONAN_PKG_LIBS_GLIB} ${CONAN_SYSTEM_LIBS_GLIB} ${CONAN_FRAMEWORKS_FOUND_GLIB})


#################
###  PIXMAN
#################
set(CONAN_PIXMAN_ROOT "C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab")
set(CONAN_INCLUDE_DIRS_PIXMAN "C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include"
			"C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include/pixman-1")
set(CONAN_LIB_DIRS_PIXMAN "C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib")
set(CONAN_BIN_DIRS_PIXMAN )
set(CONAN_RES_DIRS_PIXMAN )
set(CONAN_SRC_DIRS_PIXMAN )
set(CONAN_BUILD_DIRS_PIXMAN "C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/")
set(CONAN_FRAMEWORK_DIRS_PIXMAN )
set(CONAN_LIBS_PIXMAN libpixman-1)
set(CONAN_PKG_LIBS_PIXMAN libpixman-1)
set(CONAN_SYSTEM_LIBS_PIXMAN )
set(CONAN_FRAMEWORKS_PIXMAN )
set(CONAN_FRAMEWORKS_FOUND_PIXMAN "")  # Will be filled later
set(CONAN_DEFINES_PIXMAN )
set(CONAN_BUILD_MODULES_PATHS_PIXMAN )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_PIXMAN )

set(CONAN_C_FLAGS_PIXMAN "")
set(CONAN_CXX_FLAGS_PIXMAN "")
set(CONAN_SHARED_LINKER_FLAGS_PIXMAN "")
set(CONAN_EXE_LINKER_FLAGS_PIXMAN "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_PIXMAN_LIST "")
set(CONAN_CXX_FLAGS_PIXMAN_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_PIXMAN_LIST "")
set(CONAN_EXE_LINKER_FLAGS_PIXMAN_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_PIXMAN "${CONAN_FRAMEWORKS_PIXMAN}" "_PIXMAN" "")
# Append to aggregated values variable
set(CONAN_LIBS_PIXMAN ${CONAN_PKG_LIBS_PIXMAN} ${CONAN_SYSTEM_LIBS_PIXMAN} ${CONAN_FRAMEWORKS_FOUND_PIXMAN})


#################
###  LZO
#################
set(CONAN_LZO_ROOT "C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab")
set(CONAN_INCLUDE_DIRS_LZO "C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include"
			"C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include/lzo")
set(CONAN_LIB_DIRS_LZO "C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib")
set(CONAN_BIN_DIRS_LZO )
set(CONAN_RES_DIRS_LZO )
set(CONAN_SRC_DIRS_LZO )
set(CONAN_BUILD_DIRS_LZO "C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/")
set(CONAN_FRAMEWORK_DIRS_LZO )
set(CONAN_LIBS_LZO lzo2)
set(CONAN_PKG_LIBS_LZO lzo2)
set(CONAN_SYSTEM_LIBS_LZO )
set(CONAN_FRAMEWORKS_LZO )
set(CONAN_FRAMEWORKS_FOUND_LZO "")  # Will be filled later
set(CONAN_DEFINES_LZO )
set(CONAN_BUILD_MODULES_PATHS_LZO )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_LZO )

set(CONAN_C_FLAGS_LZO "")
set(CONAN_CXX_FLAGS_LZO "")
set(CONAN_SHARED_LINKER_FLAGS_LZO "")
set(CONAN_EXE_LINKER_FLAGS_LZO "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_LZO_LIST "")
set(CONAN_CXX_FLAGS_LZO_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_LZO_LIST "")
set(CONAN_EXE_LINKER_FLAGS_LZO_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_LZO "${CONAN_FRAMEWORKS_LZO}" "_LZO" "")
# Append to aggregated values variable
set(CONAN_LIBS_LZO ${CONAN_PKG_LIBS_LZO} ${CONAN_SYSTEM_LIBS_LZO} ${CONAN_FRAMEWORKS_FOUND_LZO})


#################
###  FONTCONFIG
#################
set(CONAN_FONTCONFIG_ROOT "C:/Users/FLUX/.conan/data/fontconfig/2.13.93/_/_/package/7bdcd0ebc5985e64875ce195eb75728a18edbb87")
set(CONAN_INCLUDE_DIRS_FONTCONFIG "C:/Users/FLUX/.conan/data/fontconfig/2.13.93/_/_/package/7bdcd0ebc5985e64875ce195eb75728a18edbb87/include")
set(CONAN_LIB_DIRS_FONTCONFIG "C:/Users/FLUX/.conan/data/fontconfig/2.13.93/_/_/package/7bdcd0ebc5985e64875ce195eb75728a18edbb87/lib")
set(CONAN_BIN_DIRS_FONTCONFIG )
set(CONAN_RES_DIRS_FONTCONFIG )
set(CONAN_SRC_DIRS_FONTCONFIG )
set(CONAN_BUILD_DIRS_FONTCONFIG "C:/Users/FLUX/.conan/data/fontconfig/2.13.93/_/_/package/7bdcd0ebc5985e64875ce195eb75728a18edbb87/")
set(CONAN_FRAMEWORK_DIRS_FONTCONFIG )
set(CONAN_LIBS_FONTCONFIG fontconfig)
set(CONAN_PKG_LIBS_FONTCONFIG fontconfig)
set(CONAN_SYSTEM_LIBS_FONTCONFIG )
set(CONAN_FRAMEWORKS_FONTCONFIG )
set(CONAN_FRAMEWORKS_FOUND_FONTCONFIG "")  # Will be filled later
set(CONAN_DEFINES_FONTCONFIG )
set(CONAN_BUILD_MODULES_PATHS_FONTCONFIG )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_FONTCONFIG )

set(CONAN_C_FLAGS_FONTCONFIG "")
set(CONAN_CXX_FLAGS_FONTCONFIG "")
set(CONAN_SHARED_LINKER_FLAGS_FONTCONFIG "")
set(CONAN_EXE_LINKER_FLAGS_FONTCONFIG "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_FONTCONFIG_LIST "")
set(CONAN_CXX_FLAGS_FONTCONFIG_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_LIST "")
set(CONAN_EXE_LINKER_FLAGS_FONTCONFIG_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_FONTCONFIG "${CONAN_FRAMEWORKS_FONTCONFIG}" "_FONTCONFIG" "")
# Append to aggregated values variable
set(CONAN_LIBS_FONTCONFIG ${CONAN_PKG_LIBS_FONTCONFIG} ${CONAN_SYSTEM_LIBS_FONTCONFIG} ${CONAN_FRAMEWORKS_FOUND_FONTCONFIG})


#################
###  LIBFFI
#################
set(CONAN_LIBFFI_ROOT "C:/Users/FLUX/.conan/data/libffi/3.4.3/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab")
set(CONAN_INCLUDE_DIRS_LIBFFI "C:/Users/FLUX/.conan/data/libffi/3.4.3/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include")
set(CONAN_LIB_DIRS_LIBFFI "C:/Users/FLUX/.conan/data/libffi/3.4.3/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib")
set(CONAN_BIN_DIRS_LIBFFI )
set(CONAN_RES_DIRS_LIBFFI )
set(CONAN_SRC_DIRS_LIBFFI )
set(CONAN_BUILD_DIRS_LIBFFI "C:/Users/FLUX/.conan/data/libffi/3.4.3/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/")
set(CONAN_FRAMEWORK_DIRS_LIBFFI )
set(CONAN_LIBS_LIBFFI libffi)
set(CONAN_PKG_LIBS_LIBFFI libffi)
set(CONAN_SYSTEM_LIBS_LIBFFI )
set(CONAN_FRAMEWORKS_LIBFFI )
set(CONAN_FRAMEWORKS_FOUND_LIBFFI "")  # Will be filled later
set(CONAN_DEFINES_LIBFFI "-DFFI_BUILDING")
set(CONAN_BUILD_MODULES_PATHS_LIBFFI )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_LIBFFI "FFI_BUILDING")

set(CONAN_C_FLAGS_LIBFFI "")
set(CONAN_CXX_FLAGS_LIBFFI "")
set(CONAN_SHARED_LINKER_FLAGS_LIBFFI "")
set(CONAN_EXE_LINKER_FLAGS_LIBFFI "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_LIBFFI_LIST "")
set(CONAN_CXX_FLAGS_LIBFFI_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_LIBFFI_LIST "")
set(CONAN_EXE_LINKER_FLAGS_LIBFFI_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_LIBFFI "${CONAN_FRAMEWORKS_LIBFFI}" "_LIBFFI" "")
# Append to aggregated values variable
set(CONAN_LIBS_LIBFFI ${CONAN_PKG_LIBS_LIBFFI} ${CONAN_SYSTEM_LIBS_LIBFFI} ${CONAN_FRAMEWORKS_FOUND_LIBFFI})


#################
###  PCRE2
#################
set(CONAN_PCRE2_ROOT "C:/Users/FLUX/.conan/data/pcre2/10.40/_/_/package/4958b335ae23c49b138435b977645c6c321f9662")
set(CONAN_INCLUDE_DIRS_PCRE2 "C:/Users/FLUX/.conan/data/pcre2/10.40/_/_/package/4958b335ae23c49b138435b977645c6c321f9662/include")
set(CONAN_LIB_DIRS_PCRE2 "C:/Users/FLUX/.conan/data/pcre2/10.40/_/_/package/4958b335ae23c49b138435b977645c6c321f9662/lib")
set(CONAN_BIN_DIRS_PCRE2 "C:/Users/FLUX/.conan/data/pcre2/10.40/_/_/package/4958b335ae23c49b138435b977645c6c321f9662/bin")
set(CONAN_RES_DIRS_PCRE2 )
set(CONAN_SRC_DIRS_PCRE2 )
set(CONAN_BUILD_DIRS_PCRE2 )
set(CONAN_FRAMEWORK_DIRS_PCRE2 )
set(CONAN_LIBS_PCRE2 pcre2-posix-static pcre2-8-static pcre2-16-static pcre2-32-static)
set(CONAN_PKG_LIBS_PCRE2 pcre2-posix-static pcre2-8-static pcre2-16-static pcre2-32-static)
set(CONAN_SYSTEM_LIBS_PCRE2 )
set(CONAN_FRAMEWORKS_PCRE2 )
set(CONAN_FRAMEWORKS_FOUND_PCRE2 "")  # Will be filled later
set(CONAN_DEFINES_PCRE2 "-DPCRE2_STATIC")
set(CONAN_BUILD_MODULES_PATHS_PCRE2 )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_PCRE2 "PCRE2_STATIC")

set(CONAN_C_FLAGS_PCRE2 "")
set(CONAN_CXX_FLAGS_PCRE2 "")
set(CONAN_SHARED_LINKER_FLAGS_PCRE2 "")
set(CONAN_EXE_LINKER_FLAGS_PCRE2 "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_PCRE2_LIST "")
set(CONAN_CXX_FLAGS_PCRE2_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_PCRE2_LIST "")
set(CONAN_EXE_LINKER_FLAGS_PCRE2_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_PCRE2 "${CONAN_FRAMEWORKS_PCRE2}" "_PCRE2" "")
# Append to aggregated values variable
set(CONAN_LIBS_PCRE2 ${CONAN_PKG_LIBS_PCRE2} ${CONAN_SYSTEM_LIBS_PCRE2} ${CONAN_FRAMEWORKS_FOUND_PCRE2})


#################
###  LIBGETTEXT
#################
set(CONAN_LIBGETTEXT_ROOT "C:/Users/FLUX/.conan/data/libgettext/0.21/_/_/package/b3e4dccd9a7572a5a29c77cc68475a955d494ef7")
set(CONAN_INCLUDE_DIRS_LIBGETTEXT "C:/Users/FLUX/.conan/data/libgettext/0.21/_/_/package/b3e4dccd9a7572a5a29c77cc68475a955d494ef7/include")
set(CONAN_LIB_DIRS_LIBGETTEXT "C:/Users/FLUX/.conan/data/libgettext/0.21/_/_/package/b3e4dccd9a7572a5a29c77cc68475a955d494ef7/lib")
set(CONAN_BIN_DIRS_LIBGETTEXT )
set(CONAN_RES_DIRS_LIBGETTEXT )
set(CONAN_SRC_DIRS_LIBGETTEXT )
set(CONAN_BUILD_DIRS_LIBGETTEXT "C:/Users/FLUX/.conan/data/libgettext/0.21/_/_/package/b3e4dccd9a7572a5a29c77cc68475a955d494ef7/")
set(CONAN_FRAMEWORK_DIRS_LIBGETTEXT )
set(CONAN_LIBS_LIBGETTEXT gnuintl)
set(CONAN_PKG_LIBS_LIBGETTEXT gnuintl)
set(CONAN_SYSTEM_LIBS_LIBGETTEXT )
set(CONAN_FRAMEWORKS_LIBGETTEXT )
set(CONAN_FRAMEWORKS_FOUND_LIBGETTEXT "")  # Will be filled later
set(CONAN_DEFINES_LIBGETTEXT )
set(CONAN_BUILD_MODULES_PATHS_LIBGETTEXT )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_LIBGETTEXT )

set(CONAN_C_FLAGS_LIBGETTEXT "")
set(CONAN_CXX_FLAGS_LIBGETTEXT "")
set(CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT "")
set(CONAN_EXE_LINKER_FLAGS_LIBGETTEXT "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_LIBGETTEXT_LIST "")
set(CONAN_CXX_FLAGS_LIBGETTEXT_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_LIST "")
set(CONAN_EXE_LINKER_FLAGS_LIBGETTEXT_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_LIBGETTEXT "${CONAN_FRAMEWORKS_LIBGETTEXT}" "_LIBGETTEXT" "")
# Append to aggregated values variable
set(CONAN_LIBS_LIBGETTEXT ${CONAN_PKG_LIBS_LIBGETTEXT} ${CONAN_SYSTEM_LIBS_LIBGETTEXT} ${CONAN_FRAMEWORKS_FOUND_LIBGETTEXT})


#################
###  EXPAT
#################
set(CONAN_EXPAT_ROOT "C:/Users/FLUX/.conan/data/expat/2.4.8/_/_/package/ce5788ba7e3bb7dc834e36b06df66c481f42c99a")
set(CONAN_INCLUDE_DIRS_EXPAT "C:/Users/FLUX/.conan/data/expat/2.4.8/_/_/package/ce5788ba7e3bb7dc834e36b06df66c481f42c99a/include")
set(CONAN_LIB_DIRS_EXPAT "C:/Users/FLUX/.conan/data/expat/2.4.8/_/_/package/ce5788ba7e3bb7dc834e36b06df66c481f42c99a/lib")
set(CONAN_BIN_DIRS_EXPAT )
set(CONAN_RES_DIRS_EXPAT )
set(CONAN_SRC_DIRS_EXPAT )
set(CONAN_BUILD_DIRS_EXPAT "C:/Users/FLUX/.conan/data/expat/2.4.8/_/_/package/ce5788ba7e3bb7dc834e36b06df66c481f42c99a/")
set(CONAN_FRAMEWORK_DIRS_EXPAT )
set(CONAN_LIBS_EXPAT libexpatMD)
set(CONAN_PKG_LIBS_EXPAT libexpatMD)
set(CONAN_SYSTEM_LIBS_EXPAT )
set(CONAN_FRAMEWORKS_EXPAT )
set(CONAN_FRAMEWORKS_FOUND_EXPAT "")  # Will be filled later
set(CONAN_DEFINES_EXPAT "-DXML_STATIC")
set(CONAN_BUILD_MODULES_PATHS_EXPAT )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_EXPAT "XML_STATIC")

set(CONAN_C_FLAGS_EXPAT "")
set(CONAN_CXX_FLAGS_EXPAT "")
set(CONAN_SHARED_LINKER_FLAGS_EXPAT "")
set(CONAN_EXE_LINKER_FLAGS_EXPAT "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_EXPAT_LIST "")
set(CONAN_CXX_FLAGS_EXPAT_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_EXPAT_LIST "")
set(CONAN_EXE_LINKER_FLAGS_EXPAT_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_EXPAT "${CONAN_FRAMEWORKS_EXPAT}" "_EXPAT" "")
# Append to aggregated values variable
set(CONAN_LIBS_EXPAT ${CONAN_PKG_LIBS_EXPAT} ${CONAN_SYSTEM_LIBS_EXPAT} ${CONAN_FRAMEWORKS_FOUND_EXPAT})


#################
###  FREETYPE
#################
set(CONAN_FREETYPE_ROOT "C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16")
set(CONAN_INCLUDE_DIRS_FREETYPE "C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/include"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/include/freetype2")
set(CONAN_LIB_DIRS_FREETYPE "C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/lib")
set(CONAN_BIN_DIRS_FREETYPE "C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/bin")
set(CONAN_RES_DIRS_FREETYPE )
set(CONAN_SRC_DIRS_FREETYPE )
set(CONAN_BUILD_DIRS_FREETYPE "C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/lib/cmake")
set(CONAN_FRAMEWORK_DIRS_FREETYPE )
set(CONAN_LIBS_FREETYPE freetype)
set(CONAN_PKG_LIBS_FREETYPE freetype)
set(CONAN_SYSTEM_LIBS_FREETYPE )
set(CONAN_FRAMEWORKS_FREETYPE )
set(CONAN_FRAMEWORKS_FOUND_FREETYPE "")  # Will be filled later
set(CONAN_DEFINES_FREETYPE )
set(CONAN_BUILD_MODULES_PATHS_FREETYPE )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_FREETYPE )

set(CONAN_C_FLAGS_FREETYPE "")
set(CONAN_CXX_FLAGS_FREETYPE "")
set(CONAN_SHARED_LINKER_FLAGS_FREETYPE "")
set(CONAN_EXE_LINKER_FLAGS_FREETYPE "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_FREETYPE_LIST "")
set(CONAN_CXX_FLAGS_FREETYPE_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_FREETYPE_LIST "")
set(CONAN_EXE_LINKER_FLAGS_FREETYPE_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_FREETYPE "${CONAN_FRAMEWORKS_FREETYPE}" "_FREETYPE" "")
# Append to aggregated values variable
set(CONAN_LIBS_FREETYPE ${CONAN_PKG_LIBS_FREETYPE} ${CONAN_SYSTEM_LIBS_FREETYPE} ${CONAN_FRAMEWORKS_FOUND_FREETYPE})


#################
###  BZIP2
#################
set(CONAN_BZIP2_ROOT "C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5")
set(CONAN_INCLUDE_DIRS_BZIP2 "C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/include")
set(CONAN_LIB_DIRS_BZIP2 "C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/lib")
set(CONAN_BIN_DIRS_BZIP2 "C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/bin")
set(CONAN_RES_DIRS_BZIP2 )
set(CONAN_SRC_DIRS_BZIP2 )
set(CONAN_BUILD_DIRS_BZIP2 "C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/")
set(CONAN_FRAMEWORK_DIRS_BZIP2 )
set(CONAN_LIBS_BZIP2 bz2)
set(CONAN_PKG_LIBS_BZIP2 bz2)
set(CONAN_SYSTEM_LIBS_BZIP2 )
set(CONAN_FRAMEWORKS_BZIP2 )
set(CONAN_FRAMEWORKS_FOUND_BZIP2 "")  # Will be filled later
set(CONAN_DEFINES_BZIP2 )
set(CONAN_BUILD_MODULES_PATHS_BZIP2 )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_BZIP2 )

set(CONAN_C_FLAGS_BZIP2 "")
set(CONAN_CXX_FLAGS_BZIP2 "")
set(CONAN_SHARED_LINKER_FLAGS_BZIP2 "")
set(CONAN_EXE_LINKER_FLAGS_BZIP2 "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_BZIP2_LIST "")
set(CONAN_CXX_FLAGS_BZIP2_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_BZIP2_LIST "")
set(CONAN_EXE_LINKER_FLAGS_BZIP2_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_BZIP2 "${CONAN_FRAMEWORKS_BZIP2}" "_BZIP2" "")
# Append to aggregated values variable
set(CONAN_LIBS_BZIP2 ${CONAN_PKG_LIBS_BZIP2} ${CONAN_SYSTEM_LIBS_BZIP2} ${CONAN_FRAMEWORKS_FOUND_BZIP2})


#################
###  LIBICONV
#################
set(CONAN_LIBICONV_ROOT "C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab")
set(CONAN_INCLUDE_DIRS_LIBICONV "C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include")
set(CONAN_LIB_DIRS_LIBICONV "C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib")
set(CONAN_BIN_DIRS_LIBICONV "C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/bin")
set(CONAN_RES_DIRS_LIBICONV )
set(CONAN_SRC_DIRS_LIBICONV )
set(CONAN_BUILD_DIRS_LIBICONV "C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/")
set(CONAN_FRAMEWORK_DIRS_LIBICONV )
set(CONAN_LIBS_LIBICONV iconv charset)
set(CONAN_PKG_LIBS_LIBICONV iconv charset)
set(CONAN_SYSTEM_LIBS_LIBICONV )
set(CONAN_FRAMEWORKS_LIBICONV )
set(CONAN_FRAMEWORKS_FOUND_LIBICONV "")  # Will be filled later
set(CONAN_DEFINES_LIBICONV )
set(CONAN_BUILD_MODULES_PATHS_LIBICONV )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_LIBICONV )

set(CONAN_C_FLAGS_LIBICONV "")
set(CONAN_CXX_FLAGS_LIBICONV "")
set(CONAN_SHARED_LINKER_FLAGS_LIBICONV "")
set(CONAN_EXE_LINKER_FLAGS_LIBICONV "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_LIBICONV_LIST "")
set(CONAN_CXX_FLAGS_LIBICONV_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_LIBICONV_LIST "")
set(CONAN_EXE_LINKER_FLAGS_LIBICONV_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_LIBICONV "${CONAN_FRAMEWORKS_LIBICONV}" "_LIBICONV" "")
# Append to aggregated values variable
set(CONAN_LIBS_LIBICONV ${CONAN_PKG_LIBS_LIBICONV} ${CONAN_SYSTEM_LIBS_LIBICONV} ${CONAN_FRAMEWORKS_FOUND_LIBICONV})


#################
###  LIBPNG
#################
set(CONAN_LIBPNG_ROOT "C:/Users/FLUX/.conan/data/libpng/1.6.37/_/_/package/ce650d9f1f1c1c0839cf0694a55c1351ddbed859")
set(CONAN_INCLUDE_DIRS_LIBPNG "C:/Users/FLUX/.conan/data/libpng/1.6.37/_/_/package/ce650d9f1f1c1c0839cf0694a55c1351ddbed859/include")
set(CONAN_LIB_DIRS_LIBPNG "C:/Users/FLUX/.conan/data/libpng/1.6.37/_/_/package/ce650d9f1f1c1c0839cf0694a55c1351ddbed859/lib")
set(CONAN_BIN_DIRS_LIBPNG )
set(CONAN_RES_DIRS_LIBPNG )
set(CONAN_SRC_DIRS_LIBPNG )
set(CONAN_BUILD_DIRS_LIBPNG "C:/Users/FLUX/.conan/data/libpng/1.6.37/_/_/package/ce650d9f1f1c1c0839cf0694a55c1351ddbed859/")
set(CONAN_FRAMEWORK_DIRS_LIBPNG )
set(CONAN_LIBS_LIBPNG libpng16)
set(CONAN_PKG_LIBS_LIBPNG libpng16)
set(CONAN_SYSTEM_LIBS_LIBPNG )
set(CONAN_FRAMEWORKS_LIBPNG )
set(CONAN_FRAMEWORKS_FOUND_LIBPNG "")  # Will be filled later
set(CONAN_DEFINES_LIBPNG )
set(CONAN_BUILD_MODULES_PATHS_LIBPNG )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_LIBPNG )

set(CONAN_C_FLAGS_LIBPNG "")
set(CONAN_CXX_FLAGS_LIBPNG "")
set(CONAN_SHARED_LINKER_FLAGS_LIBPNG "")
set(CONAN_EXE_LINKER_FLAGS_LIBPNG "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_LIBPNG_LIST "")
set(CONAN_CXX_FLAGS_LIBPNG_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_LIBPNG_LIST "")
set(CONAN_EXE_LINKER_FLAGS_LIBPNG_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_LIBPNG "${CONAN_FRAMEWORKS_LIBPNG}" "_LIBPNG" "")
# Append to aggregated values variable
set(CONAN_LIBS_LIBPNG ${CONAN_PKG_LIBS_LIBPNG} ${CONAN_SYSTEM_LIBS_LIBPNG} ${CONAN_FRAMEWORKS_FOUND_LIBPNG})


#################
###  BROTLI
#################
set(CONAN_BROTLI_ROOT "C:/Users/FLUX/.conan/data/brotli/1.0.9/_/_/package/7e52046413f1e13a5fa19e3d410c5b50df627c3f")
set(CONAN_INCLUDE_DIRS_BROTLI "C:/Users/FLUX/.conan/data/brotli/1.0.9/_/_/package/7e52046413f1e13a5fa19e3d410c5b50df627c3f/include"
			"C:/Users/FLUX/.conan/data/brotli/1.0.9/_/_/package/7e52046413f1e13a5fa19e3d410c5b50df627c3f/include/brotli")
set(CONAN_LIB_DIRS_BROTLI "C:/Users/FLUX/.conan/data/brotli/1.0.9/_/_/package/7e52046413f1e13a5fa19e3d410c5b50df627c3f/lib")
set(CONAN_BIN_DIRS_BROTLI )
set(CONAN_RES_DIRS_BROTLI )
set(CONAN_SRC_DIRS_BROTLI )
set(CONAN_BUILD_DIRS_BROTLI )
set(CONAN_FRAMEWORK_DIRS_BROTLI )
set(CONAN_LIBS_BROTLI brotlidec-static brotlienc-static brotlicommon-static)
set(CONAN_PKG_LIBS_BROTLI brotlidec-static brotlienc-static brotlicommon-static)
set(CONAN_SYSTEM_LIBS_BROTLI )
set(CONAN_FRAMEWORKS_BROTLI )
set(CONAN_FRAMEWORKS_FOUND_BROTLI "")  # Will be filled later
set(CONAN_DEFINES_BROTLI )
set(CONAN_BUILD_MODULES_PATHS_BROTLI )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_BROTLI )

set(CONAN_C_FLAGS_BROTLI "")
set(CONAN_CXX_FLAGS_BROTLI "")
set(CONAN_SHARED_LINKER_FLAGS_BROTLI "")
set(CONAN_EXE_LINKER_FLAGS_BROTLI "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_BROTLI_LIST "")
set(CONAN_CXX_FLAGS_BROTLI_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_BROTLI_LIST "")
set(CONAN_EXE_LINKER_FLAGS_BROTLI_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_BROTLI "${CONAN_FRAMEWORKS_BROTLI}" "_BROTLI" "")
# Append to aggregated values variable
set(CONAN_LIBS_BROTLI ${CONAN_PKG_LIBS_BROTLI} ${CONAN_SYSTEM_LIBS_BROTLI} ${CONAN_FRAMEWORKS_FOUND_BROTLI})


#################
###  ZLIB
#################
set(CONAN_ZLIB_ROOT "C:/Users/FLUX/.conan/data/zlib/1.2.12/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab")
set(CONAN_INCLUDE_DIRS_ZLIB "C:/Users/FLUX/.conan/data/zlib/1.2.12/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include")
set(CONAN_LIB_DIRS_ZLIB "C:/Users/FLUX/.conan/data/zlib/1.2.12/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib")
set(CONAN_BIN_DIRS_ZLIB )
set(CONAN_RES_DIRS_ZLIB )
set(CONAN_SRC_DIRS_ZLIB )
set(CONAN_BUILD_DIRS_ZLIB "C:/Users/FLUX/.conan/data/zlib/1.2.12/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/")
set(CONAN_FRAMEWORK_DIRS_ZLIB )
set(CONAN_LIBS_ZLIB zlib)
set(CONAN_PKG_LIBS_ZLIB zlib)
set(CONAN_SYSTEM_LIBS_ZLIB )
set(CONAN_FRAMEWORKS_ZLIB )
set(CONAN_FRAMEWORKS_FOUND_ZLIB "")  # Will be filled later
set(CONAN_DEFINES_ZLIB )
set(CONAN_BUILD_MODULES_PATHS_ZLIB )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_ZLIB )

set(CONAN_C_FLAGS_ZLIB "")
set(CONAN_CXX_FLAGS_ZLIB "")
set(CONAN_SHARED_LINKER_FLAGS_ZLIB "")
set(CONAN_EXE_LINKER_FLAGS_ZLIB "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_ZLIB_LIST "")
set(CONAN_CXX_FLAGS_ZLIB_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_ZLIB_LIST "")
set(CONAN_EXE_LINKER_FLAGS_ZLIB_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_ZLIB "${CONAN_FRAMEWORKS_ZLIB}" "_ZLIB" "")
# Append to aggregated values variable
set(CONAN_LIBS_ZLIB ${CONAN_PKG_LIBS_ZLIB} ${CONAN_SYSTEM_LIBS_ZLIB} ${CONAN_FRAMEWORKS_FOUND_ZLIB})


### Definition of global aggregated variables ###

set(CONAN_PACKAGE_NAME None)
set(CONAN_PACKAGE_VERSION None)

set(CONAN_SETTINGS_ARCH "x86_64")
set(CONAN_SETTINGS_ARCH_BUILD "x86_64")
set(CONAN_SETTINGS_BUILD_TYPE "Release")
set(CONAN_SETTINGS_COMPILER "Visual Studio")
set(CONAN_SETTINGS_COMPILER_RUNTIME "MD")
set(CONAN_SETTINGS_COMPILER_VERSION "16")
set(CONAN_SETTINGS_OS "Windows")
set(CONAN_SETTINGS_OS_BUILD "Windows")

set(CONAN_DEPENDENCIES cairo glib pixman lzo fontconfig libffi pcre2 libgettext expat freetype bzip2 libiconv libpng brotli zlib)
# Storing original command line args (CMake helper) flags
set(CONAN_CMD_CXX_FLAGS ${CONAN_CXX_FLAGS})

set(CONAN_CMD_SHARED_LINKER_FLAGS ${CONAN_SHARED_LINKER_FLAGS})
set(CONAN_CMD_C_FLAGS ${CONAN_C_FLAGS})
# Defining accumulated conan variables for all deps

set(CONAN_INCLUDE_DIRS "C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/include"
			"C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/include/cairo"
			"C:/.conan/ecd301/1/include"
			"C:/.conan/ecd301/1/include/gio-win32-2.0"
			"C:/.conan/ecd301/1/include/glib-2.0"
			"C:/.conan/ecd301/1/lib/glib-2.0/include"
			"C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include"
			"C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include/pixman-1"
			"C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include"
			"C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include/lzo"
			"C:/Users/FLUX/.conan/data/fontconfig/2.13.93/_/_/package/7bdcd0ebc5985e64875ce195eb75728a18edbb87/include"
			"C:/Users/FLUX/.conan/data/libffi/3.4.3/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include"
			"C:/Users/FLUX/.conan/data/pcre2/10.40/_/_/package/4958b335ae23c49b138435b977645c6c321f9662/include"
			"C:/Users/FLUX/.conan/data/libgettext/0.21/_/_/package/b3e4dccd9a7572a5a29c77cc68475a955d494ef7/include"
			"C:/Users/FLUX/.conan/data/expat/2.4.8/_/_/package/ce5788ba7e3bb7dc834e36b06df66c481f42c99a/include"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/include"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/include/freetype2"
			"C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/include"
			"C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include"
			"C:/Users/FLUX/.conan/data/libpng/1.6.37/_/_/package/ce650d9f1f1c1c0839cf0694a55c1351ddbed859/include"
			"C:/Users/FLUX/.conan/data/brotli/1.0.9/_/_/package/7e52046413f1e13a5fa19e3d410c5b50df627c3f/include"
			"C:/Users/FLUX/.conan/data/brotli/1.0.9/_/_/package/7e52046413f1e13a5fa19e3d410c5b50df627c3f/include/brotli"
			"C:/Users/FLUX/.conan/data/zlib/1.2.12/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/include" ${CONAN_INCLUDE_DIRS})
set(CONAN_LIB_DIRS "C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/lib"
			"C:/.conan/ecd301/1/lib"
			"C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib"
			"C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib"
			"C:/Users/FLUX/.conan/data/fontconfig/2.13.93/_/_/package/7bdcd0ebc5985e64875ce195eb75728a18edbb87/lib"
			"C:/Users/FLUX/.conan/data/libffi/3.4.3/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib"
			"C:/Users/FLUX/.conan/data/pcre2/10.40/_/_/package/4958b335ae23c49b138435b977645c6c321f9662/lib"
			"C:/Users/FLUX/.conan/data/libgettext/0.21/_/_/package/b3e4dccd9a7572a5a29c77cc68475a955d494ef7/lib"
			"C:/Users/FLUX/.conan/data/expat/2.4.8/_/_/package/ce5788ba7e3bb7dc834e36b06df66c481f42c99a/lib"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/lib"
			"C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/lib"
			"C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib"
			"C:/Users/FLUX/.conan/data/libpng/1.6.37/_/_/package/ce650d9f1f1c1c0839cf0694a55c1351ddbed859/lib"
			"C:/Users/FLUX/.conan/data/brotli/1.0.9/_/_/package/7e52046413f1e13a5fa19e3d410c5b50df627c3f/lib"
			"C:/Users/FLUX/.conan/data/zlib/1.2.12/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/lib" ${CONAN_LIB_DIRS})
set(CONAN_BIN_DIRS "C:/.conan/ecd301/1/bin"
			"C:/Users/FLUX/.conan/data/pcre2/10.40/_/_/package/4958b335ae23c49b138435b977645c6c321f9662/bin"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/bin"
			"C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/bin"
			"C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/bin" ${CONAN_BIN_DIRS})
set(CONAN_RES_DIRS  ${CONAN_RES_DIRS})
set(CONAN_FRAMEWORK_DIRS  ${CONAN_FRAMEWORK_DIRS})
set(CONAN_LIBS cairo-script-interpreter cairo-gobject cairo gthread-2.0 gio-2.0 gmodule-2.0 gobject-2.0 glib-2.0 libpixman-1 lzo2 fontconfig libffi pcre2-posix-static pcre2-8-static pcre2-16-static pcre2-32-static gnuintl libexpatMD freetype bz2 iconv charset libpng16 brotlidec-static brotlienc-static brotlicommon-static zlib ${CONAN_LIBS})
set(CONAN_PKG_LIBS cairo-script-interpreter cairo-gobject cairo gthread-2.0 gio-2.0 gmodule-2.0 gobject-2.0 glib-2.0 libpixman-1 lzo2 fontconfig libffi pcre2-posix-static pcre2-8-static pcre2-16-static pcre2-32-static gnuintl libexpatMD freetype bz2 iconv charset libpng16 brotlidec-static brotlienc-static brotlicommon-static zlib ${CONAN_PKG_LIBS})
set(CONAN_SYSTEM_LIBS gdi32 msimg32 iphlpapi dnsapi shlwapi ws2_32 ole32 shell32 user32 advapi32 ${CONAN_SYSTEM_LIBS})
set(CONAN_FRAMEWORKS  ${CONAN_FRAMEWORKS})
set(CONAN_FRAMEWORKS_FOUND "")  # Will be filled later
set(CONAN_DEFINES "-DXML_STATIC"
			"-DPCRE2_STATIC"
			"-DFFI_BUILDING"
			"-DCAIRO_WIN32_STATIC_BUILD=1" ${CONAN_DEFINES})
set(CONAN_BUILD_MODULES_PATHS  ${CONAN_BUILD_MODULES_PATHS})
set(CONAN_CMAKE_MODULE_PATH "C:/Users/FLUX/.conan/data/cairo/1.17.4/_/_/package/4734f292790887cbde7cd683db4b2680da608222/"
			"C:/Users/FLUX/.conan/data/pixman/0.40.0/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/"
			"C:/Users/FLUX/.conan/data/lzo/2.10/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/"
			"C:/Users/FLUX/.conan/data/fontconfig/2.13.93/_/_/package/7bdcd0ebc5985e64875ce195eb75728a18edbb87/"
			"C:/Users/FLUX/.conan/data/libffi/3.4.3/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/"
			"C:/Users/FLUX/.conan/data/libgettext/0.21/_/_/package/b3e4dccd9a7572a5a29c77cc68475a955d494ef7/"
			"C:/Users/FLUX/.conan/data/expat/2.4.8/_/_/package/ce5788ba7e3bb7dc834e36b06df66c481f42c99a/"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/"
			"C:/Users/FLUX/.conan/data/freetype/2.12.1/_/_/package/27b2733304cef577b19f699fec3a5bdbefb36d16/lib/cmake"
			"C:/Users/FLUX/.conan/data/bzip2/1.0.8/_/_/package/d16a91eadaaf5829b928b12d2f836ff7680d3df5/"
			"C:/Users/FLUX/.conan/data/libiconv/1.17/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/"
			"C:/Users/FLUX/.conan/data/libpng/1.6.37/_/_/package/ce650d9f1f1c1c0839cf0694a55c1351ddbed859/"
			"C:/Users/FLUX/.conan/data/zlib/1.2.12/_/_/package/3fb49604f9c2f729b85ba3115852006824e72cab/" ${CONAN_CMAKE_MODULE_PATH})

set(CONAN_CXX_FLAGS " ${CONAN_CXX_FLAGS}")
set(CONAN_SHARED_LINKER_FLAGS " ${CONAN_SHARED_LINKER_FLAGS}")
set(CONAN_EXE_LINKER_FLAGS " ${CONAN_EXE_LINKER_FLAGS}")
set(CONAN_C_FLAGS " ${CONAN_C_FLAGS}")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND "${CONAN_FRAMEWORKS}" "" "")
# Append to aggregated values variable: Use CONAN_LIBS instead of CONAN_PKG_LIBS to include user appended vars
set(CONAN_LIBS ${CONAN_LIBS} ${CONAN_SYSTEM_LIBS} ${CONAN_FRAMEWORKS_FOUND})


###  Definition of macros and functions ###

macro(conan_define_targets)
    if(${CMAKE_VERSION} VERSION_LESS "3.1.2")
        message(FATAL_ERROR "TARGETS not supported by your CMake version!")
    endif()  # CMAKE > 3.x
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CONAN_CMD_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CONAN_CMD_C_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CONAN_CMD_SHARED_LINKER_FLAGS}")


    set(_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES "${CONAN_SYSTEM_LIBS_CAIRO} ${CONAN_FRAMEWORKS_FOUND_CAIRO} CONAN_PKG::pixman CONAN_PKG::expat CONAN_PKG::lzo CONAN_PKG::zlib CONAN_PKG::freetype CONAN_PKG::fontconfig CONAN_PKG::libpng CONAN_PKG::glib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_CAIRO_DEPENDENCIES "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_CAIRO}" "${CONAN_LIB_DIRS_CAIRO}"
                                  CONAN_PACKAGE_TARGETS_CAIRO "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES}"
                                  "" cairo)
    set(_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_CAIRO_DEBUG} ${CONAN_FRAMEWORKS_FOUND_CAIRO_DEBUG} CONAN_PKG::pixman CONAN_PKG::expat CONAN_PKG::lzo CONAN_PKG::zlib CONAN_PKG::freetype CONAN_PKG::fontconfig CONAN_PKG::libpng CONAN_PKG::glib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_CAIRO_DEBUG}" "${CONAN_LIB_DIRS_CAIRO_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_CAIRO_DEBUG "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_DEBUG}"
                                  "debug" cairo)
    set(_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_CAIRO_RELEASE} ${CONAN_FRAMEWORKS_FOUND_CAIRO_RELEASE} CONAN_PKG::pixman CONAN_PKG::expat CONAN_PKG::lzo CONAN_PKG::zlib CONAN_PKG::freetype CONAN_PKG::fontconfig CONAN_PKG::libpng CONAN_PKG::glib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_CAIRO_RELEASE}" "${CONAN_LIB_DIRS_CAIRO_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_CAIRO_RELEASE "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELEASE}"
                                  "release" cairo)
    set(_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_CAIRO_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_CAIRO_RELWITHDEBINFO} CONAN_PKG::pixman CONAN_PKG::expat CONAN_PKG::lzo CONAN_PKG::zlib CONAN_PKG::freetype CONAN_PKG::fontconfig CONAN_PKG::libpng CONAN_PKG::glib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_CAIRO_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_CAIRO_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_CAIRO_RELWITHDEBINFO "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" cairo)
    set(_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_CAIRO_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_CAIRO_MINSIZEREL} CONAN_PKG::pixman CONAN_PKG::expat CONAN_PKG::lzo CONAN_PKG::zlib CONAN_PKG::freetype CONAN_PKG::fontconfig CONAN_PKG::libpng CONAN_PKG::glib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_CAIRO_MINSIZEREL}" "${CONAN_LIB_DIRS_CAIRO_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_CAIRO_MINSIZEREL "${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" cairo)

    add_library(CONAN_PKG::cairo INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::cairo PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_CAIRO} ${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_CAIRO_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_CAIRO_RELEASE} ${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_CAIRO_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_CAIRO_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_CAIRO_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_CAIRO_MINSIZEREL} ${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_CAIRO_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_CAIRO_DEBUG} ${_CONAN_PKG_LIBS_CAIRO_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_CAIRO_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_CAIRO_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::cairo PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_CAIRO}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_CAIRO_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_CAIRO_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_CAIRO_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_CAIRO_DEBUG}>)
    set_property(TARGET CONAN_PKG::cairo PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_CAIRO}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_CAIRO_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_CAIRO_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_CAIRO_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_CAIRO_DEBUG}>)
    set_property(TARGET CONAN_PKG::cairo PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_CAIRO_LIST} ${CONAN_CXX_FLAGS_CAIRO_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_CAIRO_RELEASE_LIST} ${CONAN_CXX_FLAGS_CAIRO_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_CAIRO_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_CAIRO_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_CAIRO_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_CAIRO_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_CAIRO_DEBUG_LIST}  ${CONAN_CXX_FLAGS_CAIRO_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_GLIB_DEPENDENCIES "${CONAN_SYSTEM_LIBS_GLIB} ${CONAN_FRAMEWORKS_FOUND_GLIB} CONAN_PKG::zlib CONAN_PKG::libffi CONAN_PKG::pcre2 CONAN_PKG::libgettext")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_GLIB_DEPENDENCIES "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_GLIB}" "${CONAN_LIB_DIRS_GLIB}"
                                  CONAN_PACKAGE_TARGETS_GLIB "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES}"
                                  "" glib)
    set(_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_GLIB_DEBUG} ${CONAN_FRAMEWORKS_FOUND_GLIB_DEBUG} CONAN_PKG::zlib CONAN_PKG::libffi CONAN_PKG::pcre2 CONAN_PKG::libgettext")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_GLIB_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_GLIB_DEBUG}" "${CONAN_LIB_DIRS_GLIB_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_GLIB_DEBUG "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_DEBUG}"
                                  "debug" glib)
    set(_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_GLIB_RELEASE} ${CONAN_FRAMEWORKS_FOUND_GLIB_RELEASE} CONAN_PKG::zlib CONAN_PKG::libffi CONAN_PKG::pcre2 CONAN_PKG::libgettext")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_GLIB_RELEASE}" "${CONAN_LIB_DIRS_GLIB_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_GLIB_RELEASE "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELEASE}"
                                  "release" glib)
    set(_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_GLIB_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_GLIB_RELWITHDEBINFO} CONAN_PKG::zlib CONAN_PKG::libffi CONAN_PKG::pcre2 CONAN_PKG::libgettext")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_GLIB_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_GLIB_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_GLIB_RELWITHDEBINFO "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" glib)
    set(_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_GLIB_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_GLIB_MINSIZEREL} CONAN_PKG::zlib CONAN_PKG::libffi CONAN_PKG::pcre2 CONAN_PKG::libgettext")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_GLIB_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_GLIB_MINSIZEREL}" "${CONAN_LIB_DIRS_GLIB_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_GLIB_MINSIZEREL "${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" glib)

    add_library(CONAN_PKG::glib INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::glib PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_GLIB} ${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_GLIB_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_GLIB_RELEASE} ${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_GLIB_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_GLIB_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_GLIB_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_GLIB_MINSIZEREL} ${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_GLIB_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_GLIB_DEBUG} ${_CONAN_PKG_LIBS_GLIB_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_GLIB_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_GLIB_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::glib PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_GLIB}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_GLIB_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_GLIB_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_GLIB_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_GLIB_DEBUG}>)
    set_property(TARGET CONAN_PKG::glib PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_GLIB}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_GLIB_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_GLIB_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_GLIB_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_GLIB_DEBUG}>)
    set_property(TARGET CONAN_PKG::glib PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_GLIB_LIST} ${CONAN_CXX_FLAGS_GLIB_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_GLIB_RELEASE_LIST} ${CONAN_CXX_FLAGS_GLIB_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_GLIB_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_GLIB_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_GLIB_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_GLIB_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_GLIB_DEBUG_LIST}  ${CONAN_CXX_FLAGS_GLIB_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES "${CONAN_SYSTEM_LIBS_PIXMAN} ${CONAN_FRAMEWORKS_FOUND_PIXMAN} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PIXMAN}" "${CONAN_LIB_DIRS_PIXMAN}"
                                  CONAN_PACKAGE_TARGETS_PIXMAN "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES}"
                                  "" pixman)
    set(_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_PIXMAN_DEBUG} ${CONAN_FRAMEWORKS_FOUND_PIXMAN_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PIXMAN_DEBUG}" "${CONAN_LIB_DIRS_PIXMAN_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_PIXMAN_DEBUG "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_DEBUG}"
                                  "debug" pixman)
    set(_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_PIXMAN_RELEASE} ${CONAN_FRAMEWORKS_FOUND_PIXMAN_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PIXMAN_RELEASE}" "${CONAN_LIB_DIRS_PIXMAN_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_PIXMAN_RELEASE "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELEASE}"
                                  "release" pixman)
    set(_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_PIXMAN_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_PIXMAN_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PIXMAN_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_PIXMAN_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_PIXMAN_RELWITHDEBINFO "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" pixman)
    set(_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_PIXMAN_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_PIXMAN_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PIXMAN_MINSIZEREL}" "${CONAN_LIB_DIRS_PIXMAN_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_PIXMAN_MINSIZEREL "${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" pixman)

    add_library(CONAN_PKG::pixman INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::pixman PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_PIXMAN} ${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PIXMAN_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_PIXMAN_RELEASE} ${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PIXMAN_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_PIXMAN_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PIXMAN_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_PIXMAN_MINSIZEREL} ${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PIXMAN_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_PIXMAN_DEBUG} ${_CONAN_PKG_LIBS_PIXMAN_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PIXMAN_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PIXMAN_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::pixman PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_PIXMAN}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_PIXMAN_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_PIXMAN_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_PIXMAN_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_PIXMAN_DEBUG}>)
    set_property(TARGET CONAN_PKG::pixman PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_PIXMAN}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_PIXMAN_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_PIXMAN_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_PIXMAN_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_PIXMAN_DEBUG}>)
    set_property(TARGET CONAN_PKG::pixman PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_PIXMAN_LIST} ${CONAN_CXX_FLAGS_PIXMAN_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_PIXMAN_RELEASE_LIST} ${CONAN_CXX_FLAGS_PIXMAN_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_PIXMAN_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_PIXMAN_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_PIXMAN_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_PIXMAN_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_PIXMAN_DEBUG_LIST}  ${CONAN_CXX_FLAGS_PIXMAN_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_LZO_DEPENDENCIES "${CONAN_SYSTEM_LIBS_LZO} ${CONAN_FRAMEWORKS_FOUND_LZO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LZO_DEPENDENCIES "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LZO}" "${CONAN_LIB_DIRS_LZO}"
                                  CONAN_PACKAGE_TARGETS_LZO "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES}"
                                  "" lzo)
    set(_CONAN_PKG_LIBS_LZO_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_LZO_DEBUG} ${CONAN_FRAMEWORKS_FOUND_LZO_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LZO_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LZO_DEBUG}" "${CONAN_LIB_DIRS_LZO_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_LZO_DEBUG "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_DEBUG}"
                                  "debug" lzo)
    set(_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_LZO_RELEASE} ${CONAN_FRAMEWORKS_FOUND_LZO_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LZO_RELEASE}" "${CONAN_LIB_DIRS_LZO_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_LZO_RELEASE "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELEASE}"
                                  "release" lzo)
    set(_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_LZO_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_LZO_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LZO_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_LZO_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_LZO_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" lzo)
    set(_CONAN_PKG_LIBS_LZO_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_LZO_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_LZO_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LZO_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LZO_MINSIZEREL}" "${CONAN_LIB_DIRS_LZO_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_LZO_MINSIZEREL "${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" lzo)

    add_library(CONAN_PKG::lzo INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::lzo PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_LZO} ${_CONAN_PKG_LIBS_LZO_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LZO_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_LZO_RELEASE} ${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LZO_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_LZO_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LZO_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_LZO_MINSIZEREL} ${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LZO_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_LZO_DEBUG} ${_CONAN_PKG_LIBS_LZO_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LZO_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LZO_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::lzo PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_LZO}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_LZO_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_LZO_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_LZO_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_LZO_DEBUG}>)
    set_property(TARGET CONAN_PKG::lzo PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_LZO}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_LZO_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_LZO_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_LZO_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_LZO_DEBUG}>)
    set_property(TARGET CONAN_PKG::lzo PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_LZO_LIST} ${CONAN_CXX_FLAGS_LZO_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_LZO_RELEASE_LIST} ${CONAN_CXX_FLAGS_LZO_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_LZO_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_LZO_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_LZO_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_LZO_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_LZO_DEBUG_LIST}  ${CONAN_CXX_FLAGS_LZO_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES "${CONAN_SYSTEM_LIBS_FONTCONFIG} ${CONAN_FRAMEWORKS_FOUND_FONTCONFIG} CONAN_PKG::freetype CONAN_PKG::expat")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FONTCONFIG}" "${CONAN_LIB_DIRS_FONTCONFIG}"
                                  CONAN_PACKAGE_TARGETS_FONTCONFIG "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES}"
                                  "" fontconfig)
    set(_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_FONTCONFIG_DEBUG} ${CONAN_FRAMEWORKS_FOUND_FONTCONFIG_DEBUG} CONAN_PKG::freetype CONAN_PKG::expat")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FONTCONFIG_DEBUG}" "${CONAN_LIB_DIRS_FONTCONFIG_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_FONTCONFIG_DEBUG "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_DEBUG}"
                                  "debug" fontconfig)
    set(_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_FONTCONFIG_RELEASE} ${CONAN_FRAMEWORKS_FOUND_FONTCONFIG_RELEASE} CONAN_PKG::freetype CONAN_PKG::expat")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FONTCONFIG_RELEASE}" "${CONAN_LIB_DIRS_FONTCONFIG_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_FONTCONFIG_RELEASE "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELEASE}"
                                  "release" fontconfig)
    set(_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_FONTCONFIG_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_FONTCONFIG_RELWITHDEBINFO} CONAN_PKG::freetype CONAN_PKG::expat")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FONTCONFIG_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_FONTCONFIG_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_FONTCONFIG_RELWITHDEBINFO "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" fontconfig)
    set(_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_FONTCONFIG_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_FONTCONFIG_MINSIZEREL} CONAN_PKG::freetype CONAN_PKG::expat")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FONTCONFIG_MINSIZEREL}" "${CONAN_LIB_DIRS_FONTCONFIG_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_FONTCONFIG_MINSIZEREL "${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" fontconfig)

    add_library(CONAN_PKG::fontconfig INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::fontconfig PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_FONTCONFIG} ${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FONTCONFIG_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_FONTCONFIG_RELEASE} ${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FONTCONFIG_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_FONTCONFIG_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FONTCONFIG_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_FONTCONFIG_MINSIZEREL} ${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FONTCONFIG_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_FONTCONFIG_DEBUG} ${_CONAN_PKG_LIBS_FONTCONFIG_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FONTCONFIG_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FONTCONFIG_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::fontconfig PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_FONTCONFIG}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_FONTCONFIG_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_FONTCONFIG_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_FONTCONFIG_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_FONTCONFIG_DEBUG}>)
    set_property(TARGET CONAN_PKG::fontconfig PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_FONTCONFIG}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_FONTCONFIG_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_FONTCONFIG_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_FONTCONFIG_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_FONTCONFIG_DEBUG}>)
    set_property(TARGET CONAN_PKG::fontconfig PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_FONTCONFIG_LIST} ${CONAN_CXX_FLAGS_FONTCONFIG_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_FONTCONFIG_RELEASE_LIST} ${CONAN_CXX_FLAGS_FONTCONFIG_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_FONTCONFIG_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_FONTCONFIG_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_FONTCONFIG_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_FONTCONFIG_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_FONTCONFIG_DEBUG_LIST}  ${CONAN_CXX_FLAGS_FONTCONFIG_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES "${CONAN_SYSTEM_LIBS_LIBFFI} ${CONAN_FRAMEWORKS_FOUND_LIBFFI} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBFFI}" "${CONAN_LIB_DIRS_LIBFFI}"
                                  CONAN_PACKAGE_TARGETS_LIBFFI "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES}"
                                  "" libffi)
    set(_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_LIBFFI_DEBUG} ${CONAN_FRAMEWORKS_FOUND_LIBFFI_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBFFI_DEBUG}" "${CONAN_LIB_DIRS_LIBFFI_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_LIBFFI_DEBUG "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_DEBUG}"
                                  "debug" libffi)
    set(_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_LIBFFI_RELEASE} ${CONAN_FRAMEWORKS_FOUND_LIBFFI_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBFFI_RELEASE}" "${CONAN_LIB_DIRS_LIBFFI_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_LIBFFI_RELEASE "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELEASE}"
                                  "release" libffi)
    set(_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_LIBFFI_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_LIBFFI_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBFFI_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_LIBFFI_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_LIBFFI_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" libffi)
    set(_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_LIBFFI_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_LIBFFI_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBFFI_MINSIZEREL}" "${CONAN_LIB_DIRS_LIBFFI_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_LIBFFI_MINSIZEREL "${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" libffi)

    add_library(CONAN_PKG::libffi INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::libffi PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_LIBFFI} ${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBFFI_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_LIBFFI_RELEASE} ${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBFFI_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_LIBFFI_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBFFI_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_LIBFFI_MINSIZEREL} ${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBFFI_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_LIBFFI_DEBUG} ${_CONAN_PKG_LIBS_LIBFFI_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBFFI_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBFFI_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::libffi PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_LIBFFI}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_LIBFFI_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_LIBFFI_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_LIBFFI_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_LIBFFI_DEBUG}>)
    set_property(TARGET CONAN_PKG::libffi PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_LIBFFI}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_LIBFFI_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_LIBFFI_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_LIBFFI_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_LIBFFI_DEBUG}>)
    set_property(TARGET CONAN_PKG::libffi PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_LIBFFI_LIST} ${CONAN_CXX_FLAGS_LIBFFI_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_LIBFFI_RELEASE_LIST} ${CONAN_CXX_FLAGS_LIBFFI_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_LIBFFI_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_LIBFFI_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_LIBFFI_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_LIBFFI_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_LIBFFI_DEBUG_LIST}  ${CONAN_CXX_FLAGS_LIBFFI_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES "${CONAN_SYSTEM_LIBS_PCRE2} ${CONAN_FRAMEWORKS_FOUND_PCRE2} CONAN_PKG::zlib CONAN_PKG::bzip2")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PCRE2_DEPENDENCIES "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PCRE2}" "${CONAN_LIB_DIRS_PCRE2}"
                                  CONAN_PACKAGE_TARGETS_PCRE2 "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES}"
                                  "" pcre2)
    set(_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_PCRE2_DEBUG} ${CONAN_FRAMEWORKS_FOUND_PCRE2_DEBUG} CONAN_PKG::zlib CONAN_PKG::bzip2")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PCRE2_DEBUG}" "${CONAN_LIB_DIRS_PCRE2_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_PCRE2_DEBUG "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_DEBUG}"
                                  "debug" pcre2)
    set(_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_PCRE2_RELEASE} ${CONAN_FRAMEWORKS_FOUND_PCRE2_RELEASE} CONAN_PKG::zlib CONAN_PKG::bzip2")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PCRE2_RELEASE}" "${CONAN_LIB_DIRS_PCRE2_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_PCRE2_RELEASE "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELEASE}"
                                  "release" pcre2)
    set(_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_PCRE2_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_PCRE2_RELWITHDEBINFO} CONAN_PKG::zlib CONAN_PKG::bzip2")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PCRE2_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_PCRE2_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_PCRE2_RELWITHDEBINFO "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" pcre2)
    set(_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_PCRE2_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_PCRE2_MINSIZEREL} CONAN_PKG::zlib CONAN_PKG::bzip2")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_PCRE2_MINSIZEREL}" "${CONAN_LIB_DIRS_PCRE2_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_PCRE2_MINSIZEREL "${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" pcre2)

    add_library(CONAN_PKG::pcre2 INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::pcre2 PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_PCRE2} ${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PCRE2_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_PCRE2_RELEASE} ${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PCRE2_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_PCRE2_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PCRE2_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_PCRE2_MINSIZEREL} ${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PCRE2_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_PCRE2_DEBUG} ${_CONAN_PKG_LIBS_PCRE2_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_PCRE2_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_PCRE2_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::pcre2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_PCRE2}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_PCRE2_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_PCRE2_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_PCRE2_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_PCRE2_DEBUG}>)
    set_property(TARGET CONAN_PKG::pcre2 PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_PCRE2}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_PCRE2_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_PCRE2_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_PCRE2_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_PCRE2_DEBUG}>)
    set_property(TARGET CONAN_PKG::pcre2 PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_PCRE2_LIST} ${CONAN_CXX_FLAGS_PCRE2_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_PCRE2_RELEASE_LIST} ${CONAN_CXX_FLAGS_PCRE2_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_PCRE2_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_PCRE2_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_PCRE2_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_PCRE2_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_PCRE2_DEBUG_LIST}  ${CONAN_CXX_FLAGS_PCRE2_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES "${CONAN_SYSTEM_LIBS_LIBGETTEXT} ${CONAN_FRAMEWORKS_FOUND_LIBGETTEXT} CONAN_PKG::libiconv")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBGETTEXT}" "${CONAN_LIB_DIRS_LIBGETTEXT}"
                                  CONAN_PACKAGE_TARGETS_LIBGETTEXT "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES}"
                                  "" libgettext)
    set(_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_LIBGETTEXT_DEBUG} ${CONAN_FRAMEWORKS_FOUND_LIBGETTEXT_DEBUG} CONAN_PKG::libiconv")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBGETTEXT_DEBUG}" "${CONAN_LIB_DIRS_LIBGETTEXT_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_LIBGETTEXT_DEBUG "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_DEBUG}"
                                  "debug" libgettext)
    set(_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_LIBGETTEXT_RELEASE} ${CONAN_FRAMEWORKS_FOUND_LIBGETTEXT_RELEASE} CONAN_PKG::libiconv")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBGETTEXT_RELEASE}" "${CONAN_LIB_DIRS_LIBGETTEXT_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_LIBGETTEXT_RELEASE "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELEASE}"
                                  "release" libgettext)
    set(_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_LIBGETTEXT_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_LIBGETTEXT_RELWITHDEBINFO} CONAN_PKG::libiconv")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBGETTEXT_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_LIBGETTEXT_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_LIBGETTEXT_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" libgettext)
    set(_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_LIBGETTEXT_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_LIBGETTEXT_MINSIZEREL} CONAN_PKG::libiconv")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBGETTEXT_MINSIZEREL}" "${CONAN_LIB_DIRS_LIBGETTEXT_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_LIBGETTEXT_MINSIZEREL "${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" libgettext)

    add_library(CONAN_PKG::libgettext INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::libgettext PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_LIBGETTEXT} ${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBGETTEXT_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_LIBGETTEXT_RELEASE} ${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBGETTEXT_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_LIBGETTEXT_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBGETTEXT_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_LIBGETTEXT_MINSIZEREL} ${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBGETTEXT_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_LIBGETTEXT_DEBUG} ${_CONAN_PKG_LIBS_LIBGETTEXT_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBGETTEXT_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBGETTEXT_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::libgettext PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_LIBGETTEXT}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_LIBGETTEXT_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_LIBGETTEXT_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_LIBGETTEXT_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_LIBGETTEXT_DEBUG}>)
    set_property(TARGET CONAN_PKG::libgettext PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_LIBGETTEXT}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_LIBGETTEXT_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_LIBGETTEXT_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_LIBGETTEXT_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_LIBGETTEXT_DEBUG}>)
    set_property(TARGET CONAN_PKG::libgettext PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_LIBGETTEXT_LIST} ${CONAN_CXX_FLAGS_LIBGETTEXT_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_LIBGETTEXT_RELEASE_LIST} ${CONAN_CXX_FLAGS_LIBGETTEXT_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_LIBGETTEXT_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_LIBGETTEXT_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_LIBGETTEXT_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_LIBGETTEXT_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_LIBGETTEXT_DEBUG_LIST}  ${CONAN_CXX_FLAGS_LIBGETTEXT_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES "${CONAN_SYSTEM_LIBS_EXPAT} ${CONAN_FRAMEWORKS_FOUND_EXPAT} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_EXPAT_DEPENDENCIES "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_EXPAT}" "${CONAN_LIB_DIRS_EXPAT}"
                                  CONAN_PACKAGE_TARGETS_EXPAT "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES}"
                                  "" expat)
    set(_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_EXPAT_DEBUG} ${CONAN_FRAMEWORKS_FOUND_EXPAT_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_EXPAT_DEBUG}" "${CONAN_LIB_DIRS_EXPAT_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_EXPAT_DEBUG "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_DEBUG}"
                                  "debug" expat)
    set(_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_EXPAT_RELEASE} ${CONAN_FRAMEWORKS_FOUND_EXPAT_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_EXPAT_RELEASE}" "${CONAN_LIB_DIRS_EXPAT_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_EXPAT_RELEASE "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELEASE}"
                                  "release" expat)
    set(_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_EXPAT_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_EXPAT_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_EXPAT_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_EXPAT_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_EXPAT_RELWITHDEBINFO "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" expat)
    set(_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_EXPAT_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_EXPAT_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_EXPAT_MINSIZEREL}" "${CONAN_LIB_DIRS_EXPAT_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_EXPAT_MINSIZEREL "${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" expat)

    add_library(CONAN_PKG::expat INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::expat PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_EXPAT} ${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_EXPAT_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_EXPAT_RELEASE} ${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_EXPAT_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_EXPAT_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_EXPAT_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_EXPAT_MINSIZEREL} ${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_EXPAT_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_EXPAT_DEBUG} ${_CONAN_PKG_LIBS_EXPAT_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_EXPAT_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_EXPAT_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::expat PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_EXPAT}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_EXPAT_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_EXPAT_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_EXPAT_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_EXPAT_DEBUG}>)
    set_property(TARGET CONAN_PKG::expat PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_EXPAT}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_EXPAT_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_EXPAT_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_EXPAT_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_EXPAT_DEBUG}>)
    set_property(TARGET CONAN_PKG::expat PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_EXPAT_LIST} ${CONAN_CXX_FLAGS_EXPAT_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_EXPAT_RELEASE_LIST} ${CONAN_CXX_FLAGS_EXPAT_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_EXPAT_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_EXPAT_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_EXPAT_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_EXPAT_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_EXPAT_DEBUG_LIST}  ${CONAN_CXX_FLAGS_EXPAT_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES "${CONAN_SYSTEM_LIBS_FREETYPE} ${CONAN_FRAMEWORKS_FOUND_FREETYPE} CONAN_PKG::libpng CONAN_PKG::zlib CONAN_PKG::bzip2 CONAN_PKG::brotli")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FREETYPE}" "${CONAN_LIB_DIRS_FREETYPE}"
                                  CONAN_PACKAGE_TARGETS_FREETYPE "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES}"
                                  "" freetype)
    set(_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_FREETYPE_DEBUG} ${CONAN_FRAMEWORKS_FOUND_FREETYPE_DEBUG} CONAN_PKG::libpng CONAN_PKG::zlib CONAN_PKG::bzip2 CONAN_PKG::brotli")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FREETYPE_DEBUG}" "${CONAN_LIB_DIRS_FREETYPE_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_FREETYPE_DEBUG "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_DEBUG}"
                                  "debug" freetype)
    set(_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_FREETYPE_RELEASE} ${CONAN_FRAMEWORKS_FOUND_FREETYPE_RELEASE} CONAN_PKG::libpng CONAN_PKG::zlib CONAN_PKG::bzip2 CONAN_PKG::brotli")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FREETYPE_RELEASE}" "${CONAN_LIB_DIRS_FREETYPE_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_FREETYPE_RELEASE "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELEASE}"
                                  "release" freetype)
    set(_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_FREETYPE_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_FREETYPE_RELWITHDEBINFO} CONAN_PKG::libpng CONAN_PKG::zlib CONAN_PKG::bzip2 CONAN_PKG::brotli")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FREETYPE_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_FREETYPE_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_FREETYPE_RELWITHDEBINFO "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" freetype)
    set(_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_FREETYPE_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_FREETYPE_MINSIZEREL} CONAN_PKG::libpng CONAN_PKG::zlib CONAN_PKG::bzip2 CONAN_PKG::brotli")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_FREETYPE_MINSIZEREL}" "${CONAN_LIB_DIRS_FREETYPE_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_FREETYPE_MINSIZEREL "${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" freetype)

    add_library(CONAN_PKG::freetype INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::freetype PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_FREETYPE} ${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FREETYPE_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_FREETYPE_RELEASE} ${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FREETYPE_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_FREETYPE_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FREETYPE_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_FREETYPE_MINSIZEREL} ${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FREETYPE_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_FREETYPE_DEBUG} ${_CONAN_PKG_LIBS_FREETYPE_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_FREETYPE_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_FREETYPE_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::freetype PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_FREETYPE}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_FREETYPE_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_FREETYPE_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_FREETYPE_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_FREETYPE_DEBUG}>)
    set_property(TARGET CONAN_PKG::freetype PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_FREETYPE}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_FREETYPE_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_FREETYPE_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_FREETYPE_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_FREETYPE_DEBUG}>)
    set_property(TARGET CONAN_PKG::freetype PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_FREETYPE_LIST} ${CONAN_CXX_FLAGS_FREETYPE_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_FREETYPE_RELEASE_LIST} ${CONAN_CXX_FLAGS_FREETYPE_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_FREETYPE_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_FREETYPE_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_FREETYPE_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_FREETYPE_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_FREETYPE_DEBUG_LIST}  ${CONAN_CXX_FLAGS_FREETYPE_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES "${CONAN_SYSTEM_LIBS_BZIP2} ${CONAN_FRAMEWORKS_FOUND_BZIP2} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BZIP2_DEPENDENCIES "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BZIP2}" "${CONAN_LIB_DIRS_BZIP2}"
                                  CONAN_PACKAGE_TARGETS_BZIP2 "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES}"
                                  "" bzip2)
    set(_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_BZIP2_DEBUG} ${CONAN_FRAMEWORKS_FOUND_BZIP2_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BZIP2_DEBUG}" "${CONAN_LIB_DIRS_BZIP2_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_BZIP2_DEBUG "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_DEBUG}"
                                  "debug" bzip2)
    set(_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_BZIP2_RELEASE} ${CONAN_FRAMEWORKS_FOUND_BZIP2_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BZIP2_RELEASE}" "${CONAN_LIB_DIRS_BZIP2_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_BZIP2_RELEASE "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELEASE}"
                                  "release" bzip2)
    set(_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_BZIP2_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_BZIP2_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BZIP2_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_BZIP2_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_BZIP2_RELWITHDEBINFO "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" bzip2)
    set(_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_BZIP2_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_BZIP2_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BZIP2_MINSIZEREL}" "${CONAN_LIB_DIRS_BZIP2_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_BZIP2_MINSIZEREL "${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" bzip2)

    add_library(CONAN_PKG::bzip2 INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::bzip2 PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_BZIP2} ${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BZIP2_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_BZIP2_RELEASE} ${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BZIP2_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_BZIP2_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BZIP2_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_BZIP2_MINSIZEREL} ${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BZIP2_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_BZIP2_DEBUG} ${_CONAN_PKG_LIBS_BZIP2_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BZIP2_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BZIP2_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::bzip2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_BZIP2}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_BZIP2_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_BZIP2_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_BZIP2_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_BZIP2_DEBUG}>)
    set_property(TARGET CONAN_PKG::bzip2 PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_BZIP2}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_BZIP2_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_BZIP2_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_BZIP2_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_BZIP2_DEBUG}>)
    set_property(TARGET CONAN_PKG::bzip2 PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_BZIP2_LIST} ${CONAN_CXX_FLAGS_BZIP2_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_BZIP2_RELEASE_LIST} ${CONAN_CXX_FLAGS_BZIP2_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_BZIP2_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_BZIP2_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_BZIP2_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_BZIP2_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_BZIP2_DEBUG_LIST}  ${CONAN_CXX_FLAGS_BZIP2_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES "${CONAN_SYSTEM_LIBS_LIBICONV} ${CONAN_FRAMEWORKS_FOUND_LIBICONV} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBICONV}" "${CONAN_LIB_DIRS_LIBICONV}"
                                  CONAN_PACKAGE_TARGETS_LIBICONV "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES}"
                                  "" libiconv)
    set(_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_LIBICONV_DEBUG} ${CONAN_FRAMEWORKS_FOUND_LIBICONV_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBICONV_DEBUG}" "${CONAN_LIB_DIRS_LIBICONV_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_LIBICONV_DEBUG "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_DEBUG}"
                                  "debug" libiconv)
    set(_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_LIBICONV_RELEASE} ${CONAN_FRAMEWORKS_FOUND_LIBICONV_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBICONV_RELEASE}" "${CONAN_LIB_DIRS_LIBICONV_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_LIBICONV_RELEASE "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELEASE}"
                                  "release" libiconv)
    set(_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_LIBICONV_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_LIBICONV_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBICONV_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_LIBICONV_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_LIBICONV_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" libiconv)
    set(_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_LIBICONV_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_LIBICONV_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBICONV_MINSIZEREL}" "${CONAN_LIB_DIRS_LIBICONV_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_LIBICONV_MINSIZEREL "${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" libiconv)

    add_library(CONAN_PKG::libiconv INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::libiconv PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_LIBICONV} ${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBICONV_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_LIBICONV_RELEASE} ${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBICONV_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_LIBICONV_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBICONV_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_LIBICONV_MINSIZEREL} ${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBICONV_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_LIBICONV_DEBUG} ${_CONAN_PKG_LIBS_LIBICONV_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBICONV_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBICONV_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::libiconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_LIBICONV}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_LIBICONV_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_LIBICONV_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_LIBICONV_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_LIBICONV_DEBUG}>)
    set_property(TARGET CONAN_PKG::libiconv PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_LIBICONV}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_LIBICONV_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_LIBICONV_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_LIBICONV_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_LIBICONV_DEBUG}>)
    set_property(TARGET CONAN_PKG::libiconv PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_LIBICONV_LIST} ${CONAN_CXX_FLAGS_LIBICONV_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_LIBICONV_RELEASE_LIST} ${CONAN_CXX_FLAGS_LIBICONV_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_LIBICONV_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_LIBICONV_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_LIBICONV_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_LIBICONV_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_LIBICONV_DEBUG_LIST}  ${CONAN_CXX_FLAGS_LIBICONV_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES "${CONAN_SYSTEM_LIBS_LIBPNG} ${CONAN_FRAMEWORKS_FOUND_LIBPNG} CONAN_PKG::zlib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBPNG}" "${CONAN_LIB_DIRS_LIBPNG}"
                                  CONAN_PACKAGE_TARGETS_LIBPNG "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES}"
                                  "" libpng)
    set(_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_LIBPNG_DEBUG} ${CONAN_FRAMEWORKS_FOUND_LIBPNG_DEBUG} CONAN_PKG::zlib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBPNG_DEBUG}" "${CONAN_LIB_DIRS_LIBPNG_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_LIBPNG_DEBUG "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_DEBUG}"
                                  "debug" libpng)
    set(_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_LIBPNG_RELEASE} ${CONAN_FRAMEWORKS_FOUND_LIBPNG_RELEASE} CONAN_PKG::zlib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBPNG_RELEASE}" "${CONAN_LIB_DIRS_LIBPNG_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_LIBPNG_RELEASE "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELEASE}"
                                  "release" libpng)
    set(_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_LIBPNG_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_LIBPNG_RELWITHDEBINFO} CONAN_PKG::zlib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBPNG_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_LIBPNG_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_LIBPNG_RELWITHDEBINFO "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" libpng)
    set(_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_LIBPNG_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_LIBPNG_MINSIZEREL} CONAN_PKG::zlib")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_LIBPNG_MINSIZEREL}" "${CONAN_LIB_DIRS_LIBPNG_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_LIBPNG_MINSIZEREL "${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" libpng)

    add_library(CONAN_PKG::libpng INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::libpng PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_LIBPNG} ${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBPNG_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_LIBPNG_RELEASE} ${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBPNG_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_LIBPNG_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBPNG_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_LIBPNG_MINSIZEREL} ${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBPNG_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_LIBPNG_DEBUG} ${_CONAN_PKG_LIBS_LIBPNG_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_LIBPNG_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_LIBPNG_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::libpng PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_LIBPNG}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_LIBPNG_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_LIBPNG_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_LIBPNG_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_LIBPNG_DEBUG}>)
    set_property(TARGET CONAN_PKG::libpng PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_LIBPNG}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_LIBPNG_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_LIBPNG_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_LIBPNG_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_LIBPNG_DEBUG}>)
    set_property(TARGET CONAN_PKG::libpng PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_LIBPNG_LIST} ${CONAN_CXX_FLAGS_LIBPNG_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_LIBPNG_RELEASE_LIST} ${CONAN_CXX_FLAGS_LIBPNG_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_LIBPNG_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_LIBPNG_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_LIBPNG_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_LIBPNG_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_LIBPNG_DEBUG_LIST}  ${CONAN_CXX_FLAGS_LIBPNG_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES "${CONAN_SYSTEM_LIBS_BROTLI} ${CONAN_FRAMEWORKS_FOUND_BROTLI} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BROTLI_DEPENDENCIES "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BROTLI}" "${CONAN_LIB_DIRS_BROTLI}"
                                  CONAN_PACKAGE_TARGETS_BROTLI "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES}"
                                  "" brotli)
    set(_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_BROTLI_DEBUG} ${CONAN_FRAMEWORKS_FOUND_BROTLI_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BROTLI_DEBUG}" "${CONAN_LIB_DIRS_BROTLI_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_BROTLI_DEBUG "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_DEBUG}"
                                  "debug" brotli)
    set(_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_BROTLI_RELEASE} ${CONAN_FRAMEWORKS_FOUND_BROTLI_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BROTLI_RELEASE}" "${CONAN_LIB_DIRS_BROTLI_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_BROTLI_RELEASE "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELEASE}"
                                  "release" brotli)
    set(_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_BROTLI_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_BROTLI_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BROTLI_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_BROTLI_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_BROTLI_RELWITHDEBINFO "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" brotli)
    set(_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_BROTLI_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_BROTLI_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_BROTLI_MINSIZEREL}" "${CONAN_LIB_DIRS_BROTLI_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_BROTLI_MINSIZEREL "${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" brotli)

    add_library(CONAN_PKG::brotli INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::brotli PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_BROTLI} ${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BROTLI_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_BROTLI_RELEASE} ${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BROTLI_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_BROTLI_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BROTLI_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_BROTLI_MINSIZEREL} ${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BROTLI_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_BROTLI_DEBUG} ${_CONAN_PKG_LIBS_BROTLI_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_BROTLI_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_BROTLI_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::brotli PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_BROTLI}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_BROTLI_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_BROTLI_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_BROTLI_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_BROTLI_DEBUG}>)
    set_property(TARGET CONAN_PKG::brotli PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_BROTLI}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_BROTLI_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_BROTLI_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_BROTLI_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_BROTLI_DEBUG}>)
    set_property(TARGET CONAN_PKG::brotli PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_BROTLI_LIST} ${CONAN_CXX_FLAGS_BROTLI_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_BROTLI_RELEASE_LIST} ${CONAN_CXX_FLAGS_BROTLI_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_BROTLI_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_BROTLI_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_BROTLI_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_BROTLI_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_BROTLI_DEBUG_LIST}  ${CONAN_CXX_FLAGS_BROTLI_DEBUG_LIST}>)


    set(_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES "${CONAN_SYSTEM_LIBS_ZLIB} ${CONAN_FRAMEWORKS_FOUND_ZLIB} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_ZLIB_DEPENDENCIES "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES}")
    conan_package_library_targets("${CONAN_PKG_LIBS_ZLIB}" "${CONAN_LIB_DIRS_ZLIB}"
                                  CONAN_PACKAGE_TARGETS_ZLIB "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES}"
                                  "" zlib)
    set(_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_DEBUG "${CONAN_SYSTEM_LIBS_ZLIB_DEBUG} ${CONAN_FRAMEWORKS_FOUND_ZLIB_DEBUG} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_DEBUG "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_DEBUG}")
    conan_package_library_targets("${CONAN_PKG_LIBS_ZLIB_DEBUG}" "${CONAN_LIB_DIRS_ZLIB_DEBUG}"
                                  CONAN_PACKAGE_TARGETS_ZLIB_DEBUG "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_DEBUG}"
                                  "debug" zlib)
    set(_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELEASE "${CONAN_SYSTEM_LIBS_ZLIB_RELEASE} ${CONAN_FRAMEWORKS_FOUND_ZLIB_RELEASE} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELEASE "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELEASE}")
    conan_package_library_targets("${CONAN_PKG_LIBS_ZLIB_RELEASE}" "${CONAN_LIB_DIRS_ZLIB_RELEASE}"
                                  CONAN_PACKAGE_TARGETS_ZLIB_RELEASE "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELEASE}"
                                  "release" zlib)
    set(_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELWITHDEBINFO "${CONAN_SYSTEM_LIBS_ZLIB_RELWITHDEBINFO} ${CONAN_FRAMEWORKS_FOUND_ZLIB_RELWITHDEBINFO} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELWITHDEBINFO "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELWITHDEBINFO}")
    conan_package_library_targets("${CONAN_PKG_LIBS_ZLIB_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_ZLIB_RELWITHDEBINFO}"
                                  CONAN_PACKAGE_TARGETS_ZLIB_RELWITHDEBINFO "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELWITHDEBINFO}"
                                  "relwithdebinfo" zlib)
    set(_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_MINSIZEREL "${CONAN_SYSTEM_LIBS_ZLIB_MINSIZEREL} ${CONAN_FRAMEWORKS_FOUND_ZLIB_MINSIZEREL} ")
    string(REPLACE " " ";" _CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_MINSIZEREL "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_MINSIZEREL}")
    conan_package_library_targets("${CONAN_PKG_LIBS_ZLIB_MINSIZEREL}" "${CONAN_LIB_DIRS_ZLIB_MINSIZEREL}"
                                  CONAN_PACKAGE_TARGETS_ZLIB_MINSIZEREL "${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_MINSIZEREL}"
                                  "minsizerel" zlib)

    add_library(CONAN_PKG::zlib INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::zlib PROPERTY INTERFACE_LINK_LIBRARIES ${CONAN_PACKAGE_TARGETS_ZLIB} ${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_ZLIB_LIST}>

                                                                 $<$<CONFIG:Release>:${CONAN_PACKAGE_TARGETS_ZLIB_RELEASE} ${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELEASE}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_RELEASE_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_ZLIB_RELEASE_LIST}>>

                                                                 $<$<CONFIG:RelWithDebInfo>:${CONAN_PACKAGE_TARGETS_ZLIB_RELWITHDEBINFO} ${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_RELWITHDEBINFO}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_RELWITHDEBINFO_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_ZLIB_RELWITHDEBINFO_LIST}>>

                                                                 $<$<CONFIG:MinSizeRel>:${CONAN_PACKAGE_TARGETS_ZLIB_MINSIZEREL} ${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_MINSIZEREL}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_MINSIZEREL_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_ZLIB_MINSIZEREL_LIST}>>

                                                                 $<$<CONFIG:Debug>:${CONAN_PACKAGE_TARGETS_ZLIB_DEBUG} ${_CONAN_PKG_LIBS_ZLIB_DEPENDENCIES_DEBUG}
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${CONAN_SHARED_LINKER_FLAGS_ZLIB_DEBUG_LIST}>
                                                                 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CONAN_EXE_LINKER_FLAGS_ZLIB_DEBUG_LIST}>>)
    set_property(TARGET CONAN_PKG::zlib PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CONAN_INCLUDE_DIRS_ZLIB}
                                                                      $<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_ZLIB_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_ZLIB_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_ZLIB_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_ZLIB_DEBUG}>)
    set_property(TARGET CONAN_PKG::zlib PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_ZLIB}
                                                                      $<$<CONFIG:Release>:${CONAN_COMPILE_DEFINITIONS_ZLIB_RELEASE}>
                                                                      $<$<CONFIG:RelWithDebInfo>:${CONAN_COMPILE_DEFINITIONS_ZLIB_RELWITHDEBINFO}>
                                                                      $<$<CONFIG:MinSizeRel>:${CONAN_COMPILE_DEFINITIONS_ZLIB_MINSIZEREL}>
                                                                      $<$<CONFIG:Debug>:${CONAN_COMPILE_DEFINITIONS_ZLIB_DEBUG}>)
    set_property(TARGET CONAN_PKG::zlib PROPERTY INTERFACE_COMPILE_OPTIONS ${CONAN_C_FLAGS_ZLIB_LIST} ${CONAN_CXX_FLAGS_ZLIB_LIST}
                                                                  $<$<CONFIG:Release>:${CONAN_C_FLAGS_ZLIB_RELEASE_LIST} ${CONAN_CXX_FLAGS_ZLIB_RELEASE_LIST}>
                                                                  $<$<CONFIG:RelWithDebInfo>:${CONAN_C_FLAGS_ZLIB_RELWITHDEBINFO_LIST} ${CONAN_CXX_FLAGS_ZLIB_RELWITHDEBINFO_LIST}>
                                                                  $<$<CONFIG:MinSizeRel>:${CONAN_C_FLAGS_ZLIB_MINSIZEREL_LIST} ${CONAN_CXX_FLAGS_ZLIB_MINSIZEREL_LIST}>
                                                                  $<$<CONFIG:Debug>:${CONAN_C_FLAGS_ZLIB_DEBUG_LIST}  ${CONAN_CXX_FLAGS_ZLIB_DEBUG_LIST}>)

    set(CONAN_TARGETS CONAN_PKG::cairo CONAN_PKG::glib CONAN_PKG::pixman CONAN_PKG::lzo CONAN_PKG::fontconfig CONAN_PKG::libffi CONAN_PKG::pcre2 CONAN_PKG::libgettext CONAN_PKG::expat CONAN_PKG::freetype CONAN_PKG::bzip2 CONAN_PKG::libiconv CONAN_PKG::libpng CONAN_PKG::brotli CONAN_PKG::zlib)

endmacro()


macro(conan_basic_setup)
    set(options TARGETS NO_OUTPUT_DIRS SKIP_RPATH KEEP_RPATHS SKIP_STD SKIP_FPIC)
    cmake_parse_arguments(ARGUMENTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(CONAN_EXPORTED)
        conan_message(STATUS "Conan: called by CMake conan helper")
    endif()

    if(CONAN_IN_LOCAL_CACHE)
        conan_message(STATUS "Conan: called inside local cache")
    endif()

    if(NOT ARGUMENTS_NO_OUTPUT_DIRS)
        conan_message(STATUS "Conan: Adjusting output directories")
        conan_output_dirs_setup()
    endif()

    if(NOT ARGUMENTS_TARGETS)
        conan_message(STATUS "Conan: Using cmake global configuration")
        conan_global_flags()
    else()
        conan_message(STATUS "Conan: Using cmake targets configuration")
        conan_define_targets()
    endif()

    if(ARGUMENTS_SKIP_RPATH)
        # Change by "DEPRECATION" or "SEND_ERROR" when we are ready
        conan_message(WARNING "Conan: SKIP_RPATH is deprecated, it has been renamed to KEEP_RPATHS")
    endif()

    if(NOT ARGUMENTS_SKIP_RPATH AND NOT ARGUMENTS_KEEP_RPATHS)
        # Parameter has renamed, but we keep the compatibility with old SKIP_RPATH
        conan_set_rpath()
    endif()

    if(NOT ARGUMENTS_SKIP_STD)
        conan_set_std()
    endif()

    if(NOT ARGUMENTS_SKIP_FPIC)
        conan_set_fpic()
    endif()

    conan_check_compiler()
    conan_set_libcxx()
    conan_set_vs_runtime()
    conan_set_find_paths()
    conan_include_build_modules()
    conan_set_find_library_paths()
endmacro()


macro(conan_set_find_paths)
    # CMAKE_MODULE_PATH does not have Debug/Release config, but there are variables
    # CONAN_CMAKE_MODULE_PATH_DEBUG to be used by the consumer
    # CMake can find findXXX.cmake files in the root of packages
    set(CMAKE_MODULE_PATH ${CONAN_CMAKE_MODULE_PATH} ${CMAKE_MODULE_PATH})

    # Make find_package() to work
    set(CMAKE_PREFIX_PATH ${CONAN_CMAKE_MODULE_PATH} ${CMAKE_PREFIX_PATH})

    # Set the find root path (cross build)
    set(CMAKE_FIND_ROOT_PATH ${CONAN_CMAKE_FIND_ROOT_PATH} ${CMAKE_FIND_ROOT_PATH})
    if(CONAN_CMAKE_FIND_ROOT_PATH_MODE_PROGRAM)
        set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ${CONAN_CMAKE_FIND_ROOT_PATH_MODE_PROGRAM})
    endif()
    if(CONAN_CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ${CONAN_CMAKE_FIND_ROOT_PATH_MODE_LIBRARY})
    endif()
    if(CONAN_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ${CONAN_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})
    endif()
endmacro()


macro(conan_set_find_library_paths)
    # CMAKE_INCLUDE_PATH, CMAKE_LIBRARY_PATH does not have Debug/Release config, but there are variables
    # CONAN_INCLUDE_DIRS_DEBUG/RELEASE CONAN_LIB_DIRS_DEBUG/RELEASE to be used by the consumer
    # For find_library
    set(CMAKE_INCLUDE_PATH ${CONAN_INCLUDE_DIRS} ${CMAKE_INCLUDE_PATH})
    set(CMAKE_LIBRARY_PATH ${CONAN_LIB_DIRS} ${CMAKE_LIBRARY_PATH})
endmacro()


macro(conan_set_vs_runtime)
    if(CONAN_LINK_RUNTIME)
        conan_get_policy(CMP0091 policy_0091)
        if(policy_0091 STREQUAL "NEW")
            if(CONAN_LINK_RUNTIME MATCHES "MTd")
                set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
            elseif(CONAN_LINK_RUNTIME MATCHES "MDd")
                set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
            elseif(CONAN_LINK_RUNTIME MATCHES "MT")
                set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
            elseif(CONAN_LINK_RUNTIME MATCHES "MD")
                set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
            endif()
        else()
            foreach(flag CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE
                         CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
                         CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL)
                if(DEFINED ${flag})
                    string(REPLACE "/MD" ${CONAN_LINK_RUNTIME} ${flag} "${${flag}}")
                endif()
            endforeach()
            foreach(flag CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
                if(DEFINED ${flag})
                    string(REPLACE "/MDd" ${CONAN_LINK_RUNTIME} ${flag} "${${flag}}")
                endif()
            endforeach()
        endif()
    endif()
endmacro()


macro(conan_flags_setup)
    # Macro maintained for backwards compatibility
    conan_set_find_library_paths()
    conan_global_flags()
    conan_set_rpath()
    conan_set_vs_runtime()
    conan_set_libcxx()
endmacro()


function(conan_message MESSAGE_OUTPUT)
    if(NOT CONAN_CMAKE_SILENT_OUTPUT)
        message(${ARGV${0}})
    endif()
endfunction()


function(conan_get_policy policy_id policy)
    if(POLICY "${policy_id}")
        cmake_policy(GET "${policy_id}" _policy)
        set(${policy} "${_policy}" PARENT_SCOPE)
    else()
        set(${policy} "" PARENT_SCOPE)
    endif()
endfunction()


function(conan_find_libraries_abs_path libraries package_libdir libraries_abs_path)
    foreach(_LIBRARY_NAME ${libraries})
        find_library(CONAN_FOUND_LIBRARY NAMES ${_LIBRARY_NAME} PATHS ${package_libdir}
                     NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
        if(CONAN_FOUND_LIBRARY)
            conan_message(STATUS "Library ${_LIBRARY_NAME} found ${CONAN_FOUND_LIBRARY}")
            set(CONAN_FULLPATH_LIBS ${CONAN_FULLPATH_LIBS} ${CONAN_FOUND_LIBRARY})
        else()
            conan_message(STATUS "Library ${_LIBRARY_NAME} not found in package, might be system one")
            set(CONAN_FULLPATH_LIBS ${CONAN_FULLPATH_LIBS} ${_LIBRARY_NAME})
        endif()
        unset(CONAN_FOUND_LIBRARY CACHE)
    endforeach()
    set(${libraries_abs_path} ${CONAN_FULLPATH_LIBS} PARENT_SCOPE)
endfunction()


function(conan_package_library_targets libraries package_libdir libraries_abs_path deps build_type package_name)
    unset(_CONAN_ACTUAL_TARGETS CACHE)
    unset(_CONAN_FOUND_SYSTEM_LIBS CACHE)
    foreach(_LIBRARY_NAME ${libraries})
        find_library(CONAN_FOUND_LIBRARY NAMES ${_LIBRARY_NAME} PATHS ${package_libdir}
                     NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
        if(CONAN_FOUND_LIBRARY)
            conan_message(STATUS "Library ${_LIBRARY_NAME} found ${CONAN_FOUND_LIBRARY}")
            set(_LIB_NAME CONAN_LIB::${package_name}_${_LIBRARY_NAME}${build_type})
            add_library(${_LIB_NAME} UNKNOWN IMPORTED)
            set_target_properties(${_LIB_NAME} PROPERTIES IMPORTED_LOCATION ${CONAN_FOUND_LIBRARY})
            set(CONAN_FULLPATH_LIBS ${CONAN_FULLPATH_LIBS} ${_LIB_NAME})
            set(_CONAN_ACTUAL_TARGETS ${_CONAN_ACTUAL_TARGETS} ${_LIB_NAME})
        else()
            conan_message(STATUS "Library ${_LIBRARY_NAME} not found in package, might be system one")
            set(CONAN_FULLPATH_LIBS ${CONAN_FULLPATH_LIBS} ${_LIBRARY_NAME})
            set(_CONAN_FOUND_SYSTEM_LIBS "${_CONAN_FOUND_SYSTEM_LIBS};${_LIBRARY_NAME}")
        endif()
        unset(CONAN_FOUND_LIBRARY CACHE)
    endforeach()

    # Add all dependencies to all targets
    string(REPLACE " " ";" deps_list "${deps}")
    foreach(_CONAN_ACTUAL_TARGET ${_CONAN_ACTUAL_TARGETS})
        set_property(TARGET ${_CONAN_ACTUAL_TARGET} PROPERTY INTERFACE_LINK_LIBRARIES "${_CONAN_FOUND_SYSTEM_LIBS};${deps_list}")
    endforeach()

    set(${libraries_abs_path} ${CONAN_FULLPATH_LIBS} PARENT_SCOPE)
endfunction()


macro(conan_set_libcxx)
    if(DEFINED CONAN_LIBCXX)
        conan_message(STATUS "Conan: C++ stdlib: ${CONAN_LIBCXX}")
        if(CONAN_COMPILER STREQUAL "clang" OR CONAN_COMPILER STREQUAL "apple-clang")
            if(CONAN_LIBCXX STREQUAL "libstdc++" OR CONAN_LIBCXX STREQUAL "libstdc++11" )
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
            elseif(CONAN_LIBCXX STREQUAL "libc++")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
            endif()
        endif()
        if(CONAN_COMPILER STREQUAL "sun-cc")
            if(CONAN_LIBCXX STREQUAL "libCstd")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -library=Cstd")
            elseif(CONAN_LIBCXX STREQUAL "libstdcxx")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -library=stdcxx4")
            elseif(CONAN_LIBCXX STREQUAL "libstlport")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -library=stlport4")
            elseif(CONAN_LIBCXX STREQUAL "libstdc++")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -library=stdcpp")
            endif()
        endif()
        if(CONAN_LIBCXX STREQUAL "libstdc++11")
            add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
        elseif(CONAN_LIBCXX STREQUAL "libstdc++")
            add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
        endif()
    endif()
endmacro()


macro(conan_set_std)
    conan_message(STATUS "Conan: Adjusting language standard")
    # Do not warn "Manually-specified variables were not used by the project"
    set(ignorevar "${CONAN_STD_CXX_FLAG}${CONAN_CMAKE_CXX_STANDARD}${CONAN_CMAKE_CXX_EXTENSIONS}")
    if (CMAKE_VERSION VERSION_LESS "3.1" OR
        (CMAKE_VERSION VERSION_LESS "3.12" AND ("${CONAN_CMAKE_CXX_STANDARD}" STREQUAL "20" OR "${CONAN_CMAKE_CXX_STANDARD}" STREQUAL "gnu20")))
        if(CONAN_STD_CXX_FLAG)
            conan_message(STATUS "Conan setting CXX_FLAGS flags: ${CONAN_STD_CXX_FLAG}")
            set(CMAKE_CXX_FLAGS "${CONAN_STD_CXX_FLAG} ${CMAKE_CXX_FLAGS}")
        endif()
    else()
        if(CONAN_CMAKE_CXX_STANDARD)
            conan_message(STATUS "Conan setting CPP STANDARD: ${CONAN_CMAKE_CXX_STANDARD} WITH EXTENSIONS ${CONAN_CMAKE_CXX_EXTENSIONS}")
            set(CMAKE_CXX_STANDARD ${CONAN_CMAKE_CXX_STANDARD})
            set(CMAKE_CXX_EXTENSIONS ${CONAN_CMAKE_CXX_EXTENSIONS})
        endif()
    endif()
endmacro()


macro(conan_set_rpath)
    conan_message(STATUS "Conan: Adjusting default RPATHs Conan policies")
    if(APPLE)
        # https://cmake.org/Wiki/CMake_RPATH_handling
        # CONAN GUIDE: All generated libraries should have the id and dependencies to other
        # dylibs without path, just the name, EX:
        # libMyLib1.dylib:
        #     libMyLib1.dylib (compatibility version 0.0.0, current version 0.0.0)
        #     libMyLib0.dylib (compatibility version 0.0.0, current version 0.0.0)
        #     /usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.0.0)
        #     /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1197.1.1)
        # AVOID RPATH FOR *.dylib, ALL LIBS BETWEEN THEM AND THE EXE
        # SHOULD BE ON THE LINKER RESOLVER PATH (./ IS ONE OF THEM)
        set(CMAKE_SKIP_RPATH 1 CACHE BOOL "rpaths" FORCE)
        # Policy CMP0068
        # We want the old behavior, in CMake >= 3.9 CMAKE_SKIP_RPATH won't affect the install_name in OSX
        set(CMAKE_INSTALL_NAME_DIR "")
    endif()
endmacro()


macro(conan_set_fpic)
    if(DEFINED CONAN_CMAKE_POSITION_INDEPENDENT_CODE)
        conan_message(STATUS "Conan: Adjusting fPIC flag (${CONAN_CMAKE_POSITION_INDEPENDENT_CODE})")
        set(CMAKE_POSITION_INDEPENDENT_CODE ${CONAN_CMAKE_POSITION_INDEPENDENT_CODE})
    endif()
endmacro()


macro(conan_output_dirs_setup)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
endmacro()


macro(conan_split_version VERSION_STRING MAJOR MINOR)
    #make a list from the version string
    string(REPLACE "." ";" VERSION_LIST "${VERSION_STRING}")

    #write output values
    list(LENGTH VERSION_LIST _version_len)
    list(GET VERSION_LIST 0 ${MAJOR})
    if(${_version_len} GREATER 1)
        list(GET VERSION_LIST 1 ${MINOR})
    endif()
endmacro()


macro(conan_error_compiler_version)
    message(FATAL_ERROR "Detected a mismatch for the compiler version between your conan profile settings and CMake: \n"
                        "Compiler version specified in your conan profile: ${CONAN_COMPILER_VERSION}\n"
                        "Compiler version detected in CMake: ${VERSION_MAJOR}.${VERSION_MINOR}\n"
                        "Please check your conan profile settings (conan profile show [default|your_profile_name])\n"
                        "P.S. You may set CONAN_DISABLE_CHECK_COMPILER CMake variable in order to disable this check."
           )
endmacro()

set(_CONAN_CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})

function(conan_get_compiler CONAN_INFO_COMPILER CONAN_INFO_COMPILER_VERSION)
    conan_message(STATUS "Current conanbuildinfo.cmake directory: " ${_CONAN_CURRENT_DIR})
    if(NOT EXISTS ${_CONAN_CURRENT_DIR}/conaninfo.txt)
        conan_message(STATUS "WARN: conaninfo.txt not found")
        return()
    endif()

    file (READ "${_CONAN_CURRENT_DIR}/conaninfo.txt" CONANINFO)

    # MATCHALL will match all, including the last one, which is the full_settings one
    string(REGEX MATCH "full_settings.*" _FULL_SETTINGS_MATCHED ${CONANINFO})
    string(REGEX MATCH "compiler=([-A-Za-z0-9_ ]+)" _MATCHED ${_FULL_SETTINGS_MATCHED})
    if(DEFINED CMAKE_MATCH_1)
        string(STRIP "${CMAKE_MATCH_1}" _CONAN_INFO_COMPILER)
        set(${CONAN_INFO_COMPILER} ${_CONAN_INFO_COMPILER} PARENT_SCOPE)
    endif()

    string(REGEX MATCH "compiler.version=([-A-Za-z0-9_.]+)" _MATCHED ${_FULL_SETTINGS_MATCHED})
    if(DEFINED CMAKE_MATCH_1)
        string(STRIP "${CMAKE_MATCH_1}" _CONAN_INFO_COMPILER_VERSION)
        set(${CONAN_INFO_COMPILER_VERSION} ${_CONAN_INFO_COMPILER_VERSION} PARENT_SCOPE)
    endif()
endfunction()


function(check_compiler_version)
    conan_split_version(${CMAKE_CXX_COMPILER_VERSION} VERSION_MAJOR VERSION_MINOR)
    if(DEFINED CONAN_SETTINGS_COMPILER_TOOLSET)
       conan_message(STATUS "Conan: Skipping compiler check: Declared 'compiler.toolset'")
       return()
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
        # MSVC_VERSION is defined since 2.8.2 at least
        # https://cmake.org/cmake/help/v2.8.2/cmake.html#variable:MSVC_VERSION
        # https://cmake.org/cmake/help/v3.14/variable/MSVC_VERSION.html
        if(
            # 1930 = VS 17.0 (v143 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "17" AND NOT((MSVC_VERSION EQUAL 1930) OR (MSVC_VERSION GREATER 1930))) OR
            # 1920-1929 = VS 16.0 (v142 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "16" AND NOT((MSVC_VERSION GREATER 1919) AND (MSVC_VERSION LESS 1930))) OR
            # 1910-1919 = VS 15.0 (v141 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "15" AND NOT((MSVC_VERSION GREATER 1909) AND (MSVC_VERSION LESS 1920))) OR
            # 1900      = VS 14.0 (v140 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "14" AND NOT(MSVC_VERSION EQUAL 1900)) OR
            # 1800      = VS 12.0 (v120 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "12" AND NOT VERSION_MAJOR STREQUAL "18") OR
            # 1700      = VS 11.0 (v110 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "11" AND NOT VERSION_MAJOR STREQUAL "17") OR
            # 1600      = VS 10.0 (v100 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "10" AND NOT VERSION_MAJOR STREQUAL "16") OR
            # 1500      = VS  9.0 (v90 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "9" AND NOT VERSION_MAJOR STREQUAL "15") OR
            # 1400      = VS  8.0 (v80 toolset)
            (CONAN_COMPILER_VERSION STREQUAL "8" AND NOT VERSION_MAJOR STREQUAL "14") OR
            # 1310      = VS  7.1, 1300      = VS  7.0
            (CONAN_COMPILER_VERSION STREQUAL "7" AND NOT VERSION_MAJOR STREQUAL "13") OR
            # 1200      = VS  6.0
            (CONAN_COMPILER_VERSION STREQUAL "6" AND NOT VERSION_MAJOR STREQUAL "12") )
            conan_error_compiler_version()
        endif()
    elseif(CONAN_COMPILER STREQUAL "gcc")
        conan_split_version(${CONAN_COMPILER_VERSION} CONAN_COMPILER_MAJOR CONAN_COMPILER_MINOR)
        set(_CHECK_VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
        set(_CONAN_VERSION ${CONAN_COMPILER_MAJOR}.${CONAN_COMPILER_MINOR})
        if(NOT ${CONAN_COMPILER_VERSION} VERSION_LESS 5.0)
            conan_message(STATUS "Conan: Compiler GCC>=5, checking major version ${CONAN_COMPILER_VERSION}")
            conan_split_version(${CONAN_COMPILER_VERSION} CONAN_COMPILER_MAJOR CONAN_COMPILER_MINOR)
            if("${CONAN_COMPILER_MINOR}" STREQUAL "")
                set(_CHECK_VERSION ${VERSION_MAJOR})
                set(_CONAN_VERSION ${CONAN_COMPILER_MAJOR})
            endif()
        endif()
        conan_message(STATUS "Conan: Checking correct version: ${_CHECK_VERSION}")
        if(NOT ${_CHECK_VERSION} VERSION_EQUAL ${_CONAN_VERSION})
            conan_error_compiler_version()
        endif()
    elseif(CONAN_COMPILER STREQUAL "clang")
        conan_split_version(${CONAN_COMPILER_VERSION} CONAN_COMPILER_MAJOR CONAN_COMPILER_MINOR)
        set(_CHECK_VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
        set(_CONAN_VERSION ${CONAN_COMPILER_MAJOR}.${CONAN_COMPILER_MINOR})
        if(NOT ${CONAN_COMPILER_VERSION} VERSION_LESS 8.0)
            conan_message(STATUS "Conan: Compiler Clang>=8, checking major version ${CONAN_COMPILER_VERSION}")
            if("${CONAN_COMPILER_MINOR}" STREQUAL "")
                set(_CHECK_VERSION ${VERSION_MAJOR})
                set(_CONAN_VERSION ${CONAN_COMPILER_MAJOR})
            endif()
        endif()
        conan_message(STATUS "Conan: Checking correct version: ${_CHECK_VERSION}")
        if(NOT ${_CHECK_VERSION} VERSION_EQUAL ${_CONAN_VERSION})
            conan_error_compiler_version()
        endif()
    elseif(CONAN_COMPILER STREQUAL "apple-clang" OR CONAN_COMPILER STREQUAL "sun-cc" OR CONAN_COMPILER STREQUAL "mcst-lcc")
        conan_split_version(${CONAN_COMPILER_VERSION} CONAN_COMPILER_MAJOR CONAN_COMPILER_MINOR)
        if(${CONAN_COMPILER_MAJOR} VERSION_GREATER_EQUAL "13" AND "${CONAN_COMPILER_MINOR}" STREQUAL "" AND ${CONAN_COMPILER_MAJOR} VERSION_EQUAL ${VERSION_MAJOR})
           # This is correct,  13.X is considered 13
        elseif(NOT ${VERSION_MAJOR}.${VERSION_MINOR} VERSION_EQUAL ${CONAN_COMPILER_MAJOR}.${CONAN_COMPILER_MINOR})
           conan_error_compiler_version()
        endif()
    elseif(CONAN_COMPILER STREQUAL "intel")
        conan_split_version(${CONAN_COMPILER_VERSION} CONAN_COMPILER_MAJOR CONAN_COMPILER_MINOR)
        if(NOT ${CONAN_COMPILER_VERSION} VERSION_LESS 19.1)
            if(NOT ${VERSION_MAJOR}.${VERSION_MINOR} VERSION_EQUAL ${CONAN_COMPILER_MAJOR}.${CONAN_COMPILER_MINOR})
               conan_error_compiler_version()
            endif()
        else()
            if(NOT ${VERSION_MAJOR} VERSION_EQUAL ${CONAN_COMPILER_MAJOR})
               conan_error_compiler_version()
            endif()
        endif()
    else()
        conan_message(STATUS "WARN: Unknown compiler '${CONAN_COMPILER}', skipping the version check...")
    endif()
endfunction()


function(conan_check_compiler)
    if(CONAN_DISABLE_CHECK_COMPILER)
        conan_message(STATUS "WARN: Disabled conan compiler checks")
        return()
    endif()
    if(NOT DEFINED CMAKE_CXX_COMPILER_ID)
        if(DEFINED CMAKE_C_COMPILER_ID)
            conan_message(STATUS "This project seems to be plain C, using '${CMAKE_C_COMPILER_ID}' compiler")
            set(CMAKE_CXX_COMPILER_ID ${CMAKE_C_COMPILER_ID})
            set(CMAKE_CXX_COMPILER_VERSION ${CMAKE_C_COMPILER_VERSION})
        else()
            message(FATAL_ERROR "This project seems to be plain C, but no compiler defined")
        endif()
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID AND NOT CMAKE_C_COMPILER_ID)
        # This use case happens when compiler is not identified by CMake, but the compilers are there and work
        conan_message(STATUS "*** WARN: CMake was not able to identify a C or C++ compiler ***")
        conan_message(STATUS "*** WARN: Disabling compiler checks. Please make sure your settings match your environment ***")
        return()
    endif()
    if(NOT DEFINED CONAN_COMPILER)
        conan_get_compiler(CONAN_COMPILER CONAN_COMPILER_VERSION)
        if(NOT DEFINED CONAN_COMPILER)
            conan_message(STATUS "WARN: CONAN_COMPILER variable not set, please make sure yourself that "
                          "your compiler and version matches your declared settings")
            return()
        endif()
    endif()

    if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL ${CMAKE_SYSTEM_NAME})
        set(CROSS_BUILDING 1)
    endif()

    # If using VS, verify toolset
    if (CONAN_COMPILER STREQUAL "Visual Studio")
        if (CONAN_SETTINGS_COMPILER_TOOLSET MATCHES "LLVM" OR
            CONAN_SETTINGS_COMPILER_TOOLSET MATCHES "llvm" OR
            CONAN_SETTINGS_COMPILER_TOOLSET MATCHES "clang" OR
            CONAN_SETTINGS_COMPILER_TOOLSET MATCHES "Clang")
            set(EXPECTED_CMAKE_CXX_COMPILER_ID "Clang")
        elseif (CONAN_SETTINGS_COMPILER_TOOLSET MATCHES "Intel")
            set(EXPECTED_CMAKE_CXX_COMPILER_ID "Intel")
        else()
            set(EXPECTED_CMAKE_CXX_COMPILER_ID "MSVC")
        endif()

        if (NOT CMAKE_CXX_COMPILER_ID MATCHES ${EXPECTED_CMAKE_CXX_COMPILER_ID})
            message(FATAL_ERROR "Incorrect '${CONAN_COMPILER}'. Toolset specifies compiler as '${EXPECTED_CMAKE_CXX_COMPILER_ID}' "
                                "but CMake detected '${CMAKE_CXX_COMPILER_ID}'")
        endif()

    # Avoid checks when cross compiling, apple-clang crashes because its APPLE but not apple-clang
    # Actually CMake is detecting "clang" when you are using apple-clang, only if CMP0025 is set to NEW will detect apple-clang
    elseif((CONAN_COMPILER STREQUAL "gcc" AND NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR
        (CONAN_COMPILER STREQUAL "apple-clang" AND NOT CROSS_BUILDING AND (NOT APPLE OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR
        (CONAN_COMPILER STREQUAL "clang" AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR
        (CONAN_COMPILER STREQUAL "sun-cc" AND NOT CMAKE_CXX_COMPILER_ID MATCHES "SunPro") )
        message(FATAL_ERROR "Incorrect '${CONAN_COMPILER}', is not the one detected by CMake: '${CMAKE_CXX_COMPILER_ID}'")
    endif()


    if(NOT DEFINED CONAN_COMPILER_VERSION)
        conan_message(STATUS "WARN: CONAN_COMPILER_VERSION variable not set, please make sure yourself "
                             "that your compiler version matches your declared settings")
        return()
    endif()
    check_compiler_version()
endfunction()


macro(conan_set_flags build_type)
    set(CMAKE_CXX_FLAGS${build_type} "${CMAKE_CXX_FLAGS${build_type}} ${CONAN_CXX_FLAGS${build_type}}")
    set(CMAKE_C_FLAGS${build_type} "${CMAKE_C_FLAGS${build_type}} ${CONAN_C_FLAGS${build_type}}")
    set(CMAKE_SHARED_LINKER_FLAGS${build_type} "${CMAKE_SHARED_LINKER_FLAGS${build_type}} ${CONAN_SHARED_LINKER_FLAGS${build_type}}")
    set(CMAKE_EXE_LINKER_FLAGS${build_type} "${CMAKE_EXE_LINKER_FLAGS${build_type}} ${CONAN_EXE_LINKER_FLAGS${build_type}}")
endmacro()


macro(conan_global_flags)
    if(CONAN_SYSTEM_INCLUDES)
        include_directories(SYSTEM ${CONAN_INCLUDE_DIRS}
                                   "$<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_RELEASE}>"
                                   "$<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_RELWITHDEBINFO}>"
                                   "$<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_MINSIZEREL}>"
                                   "$<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_DEBUG}>")
    else()
        include_directories(${CONAN_INCLUDE_DIRS}
                            "$<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_RELEASE}>"
                            "$<$<CONFIG:RelWithDebInfo>:${CONAN_INCLUDE_DIRS_RELWITHDEBINFO}>"
                            "$<$<CONFIG:MinSizeRel>:${CONAN_INCLUDE_DIRS_MINSIZEREL}>"
                            "$<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_DEBUG}>")
    endif()

    link_directories(${CONAN_LIB_DIRS})

    conan_find_libraries_abs_path("${CONAN_LIBS_DEBUG}" "${CONAN_LIB_DIRS_DEBUG}"
                                  CONAN_LIBS_DEBUG)
    conan_find_libraries_abs_path("${CONAN_LIBS_RELEASE}" "${CONAN_LIB_DIRS_RELEASE}"
                                  CONAN_LIBS_RELEASE)
    conan_find_libraries_abs_path("${CONAN_LIBS_RELWITHDEBINFO}" "${CONAN_LIB_DIRS_RELWITHDEBINFO}"
                                  CONAN_LIBS_RELWITHDEBINFO)
    conan_find_libraries_abs_path("${CONAN_LIBS_MINSIZEREL}" "${CONAN_LIB_DIRS_MINSIZEREL}"
                                  CONAN_LIBS_MINSIZEREL)

    add_compile_options(${CONAN_DEFINES}
                        "$<$<CONFIG:Debug>:${CONAN_DEFINES_DEBUG}>"
                        "$<$<CONFIG:Release>:${CONAN_DEFINES_RELEASE}>"
                        "$<$<CONFIG:RelWithDebInfo>:${CONAN_DEFINES_RELWITHDEBINFO}>"
                        "$<$<CONFIG:MinSizeRel>:${CONAN_DEFINES_MINSIZEREL}>")

    conan_set_flags("")
    conan_set_flags("_RELEASE")
    conan_set_flags("_DEBUG")

endmacro()


macro(conan_target_link_libraries target)
    if(CONAN_TARGETS)
        target_link_libraries(${target} ${CONAN_TARGETS})
    else()
        target_link_libraries(${target} ${CONAN_LIBS})
        foreach(_LIB ${CONAN_LIBS_RELEASE})
            target_link_libraries(${target} optimized ${_LIB})
        endforeach()
        foreach(_LIB ${CONAN_LIBS_DEBUG})
            target_link_libraries(${target} debug ${_LIB})
        endforeach()
    endif()
endmacro()


macro(conan_include_build_modules)
    if(CMAKE_BUILD_TYPE)
        if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
            set(CONAN_BUILD_MODULES_PATHS ${CONAN_BUILD_MODULES_PATHS_DEBUG} ${CONAN_BUILD_MODULES_PATHS})
        elseif(${CMAKE_BUILD_TYPE} MATCHES "Release")
            set(CONAN_BUILD_MODULES_PATHS ${CONAN_BUILD_MODULES_PATHS_RELEASE} ${CONAN_BUILD_MODULES_PATHS})
        elseif(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
            set(CONAN_BUILD_MODULES_PATHS ${CONAN_BUILD_MODULES_PATHS_RELWITHDEBINFO} ${CONAN_BUILD_MODULES_PATHS})
        elseif(${CMAKE_BUILD_TYPE} MATCHES "MinSizeRel")
            set(CONAN_BUILD_MODULES_PATHS ${CONAN_BUILD_MODULES_PATHS_MINSIZEREL} ${CONAN_BUILD_MODULES_PATHS})
        endif()
    endif()

    foreach(_BUILD_MODULE_PATH ${CONAN_BUILD_MODULES_PATHS})
        include(${_BUILD_MODULE_PATH})
    endforeach()
endmacro()


### Definition of user declared vars (user_info) ###

set(CONAN_USER_FREETYPE_LIBTOOL_VERSION "24.3.18")