add_subdirectory(canvas)
add_subdirectory(utils)
add_subdirectory(common)
add_subdirectory(windows)
add_subdirectory(widgets)
add_subdirectory(settings)
add_subdirectory(shape)
add_subdirectory(parser)
add_subdirectory(toolpath_exporter)
add_subdirectory(machine)
add_subdirectory(periph)
add_subdirectory(executor)
add_subdirectory(server)
add_subdirectory(debug)
add_subdirectory(osx)

target_sources(${PROJECT_NAME} PRIVATE 
  ${SRC_DIR}/globals.cpp
  ${SRC_DIR}/layer.cpp
  ${SRC_DIR}/document.cpp
  ${SRC_DIR}/command.cpp
  ${SRC_DIR}/clipboard.cpp
  ${SRC_DIR}/main_application.cpp
)