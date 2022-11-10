#include("${CMAKE_SOURCE_DIR}/build/conanbuildinfo.cmake")
#conan_basic_setup()

#find_package(Potrace REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5Gui COMPONENTS Private REQUIRED)

add_library(QxPotrace STATIC 
  src/qxpotrace.cpp
  include/qxpotrace.h
)

target_include_directories(QxPotrace PRIVATE
  ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)

target_link_libraries(QxPotrace PRIVATE Qt::Core)
target_link_libraries(QxPotrace PRIVATE Qt::Gui)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(QxPotrace PRIVATE "${CONAN_LIB_DIRS_LIBPOTRACE}/libpotrace.a")
  target_include_directories(QxPotrace PRIVATE
  ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
  ${CONAN_INCLUDE_DIRS_LIBPOTRACE}
)
else()
  find_package(Potrace REQUIRED)
  target_include_directories(QxPotrace PRIVATE
    ${POTRACE_INCLUDE_DIRS}
  )
  target_link_libraries(QxPotrace PRIVATE ${POTRACE_LIBRARIES} ${POTRACE_LIBRARY})
endif()
