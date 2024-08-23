if (WIN32)
  set(CMAKE_TOOLCHAIN_FILE "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

#find_package(Potrace REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Gui REQUIRED)
find_package(Qt6Gui COMPONENTS Private REQUIRED)

set(REL_LIBPOTRACE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libpotrace/potrace-1.16/src")
include_directories(${REL_LIBPOTRACE_INCLUDE_DIR})

add_library(QxPotrace STATIC 
  src/qxpotrace.cpp
  include/qxpotrace.h
)

target_include_directories(QxPotrace PRIVATE
  ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
)

target_link_libraries(QxPotrace PRIVATE Qt::Core)
target_link_libraries(QxPotrace PRIVATE Qt::Gui)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  message(STATUS "CONANLIB linking - ${CONAN_LIB_DIRS_LIBPOTRACE}")
  if("${CONAN_LIB_DIRS_LIBPOTRACE}" MATCHES "")
    target_link_libraries(QxPotrace PRIVATE "${REL_LIBPOTRACE_INCLUDE_DIR}/.libs/libpotrace.a")
  else()
    target_link_libraries(QxPotrace PRIVATE "${CONAN_LIB_DIRS_LIBPOTRACE}/libpotrace.a")
  endif()  
target_include_directories(QxPotrace PRIVATE
  ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
  ${CONAN_INCLUDE_DIRS_LIBPOTRACE}
)
else()
  find_package(Potrace REQUIRED)
  target_include_directories(QxPotrace PRIVATE
    ${POTRACE_INCLUDE_DIRS}
  )
  target_link_libraries(QxPotrace PRIVATE ${POTRACE_LIBRARIES} ${POTRACE_LIBRARY})
endif()
