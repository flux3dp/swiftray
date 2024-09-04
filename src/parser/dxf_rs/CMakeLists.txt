cmake_minimum_required(VERSION 3.10)

# Project name
project(dxf_rs)

# Create the library without specifying sources
find_package(Qt6 COMPONENTS Core Gui Widgets Core5Compat REQUIRED)
find_package(Boost REQUIRED)
add_library(dxf_rs STATIC)
add_subdirectory(engine)
add_subdirectory(debug)
add_subdirectory(fileio)
add_subdirectory(information)
add_subdirectory(math)
add_subdirectory(filters)
add_subdirectory(muparser)
add_subdirectory(jwwlib)

#target_include_directories(dxf_rs PRIVATE
#  ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
#)

target_include_directories(dxf_rs PRIVATE
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(dxf_rs PRIVATE Boost::boost)
target_link_libraries(dxf_rs PRIVATE Qt::Core)
target_link_libraries(dxf_rs PRIVATE Qt::Widgets)
target_link_libraries(dxf_rs PRIVATE Qt::Gui) 
target_link_libraries(dxf_rs PRIVATE Qt::Core5Compat)