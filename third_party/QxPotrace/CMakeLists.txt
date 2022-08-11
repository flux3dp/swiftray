
find_package(Potrace REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5Gui COMPONENTS Private REQUIRED)

add_library(QxPotrace STATIC 
  src/qxpotrace.cpp
  include/qxpotrace.h
)

target_include_directories(QxPotrace PRIVATE
  ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
  ${POTRACE_INCLUDE_DIRS}
)

target_link_libraries(QxPotrace PRIVATE Qt::Core)
target_link_libraries(QxPotrace PRIVATE Qt::Gui)
target_link_libraries(QxPotrace PRIVATE ${POTRACE_LIBRARIES} ${POTRACE_LIBRARY})
