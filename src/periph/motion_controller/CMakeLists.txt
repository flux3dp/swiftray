# Create motion_controller library

add_library(motion_controller STATIC)

target_sources(motion_controller PRIVATE
  motion_controller.cpp
  motion_controller_factory.cpp
  grbl_motion_controller.cpp
  bsl_motion_controller.cpp
)

target_link_libraries(motion_controller PRIVATE Qt::Core)
target_link_libraries(motion_controller PRIVATE Qt::Widgets)
target_link_libraries(motion_controller PRIVATE Qt::Gui) 
target_link_libraries(motion_controller PRIVATE Qt::SerialPort)

# Link motion_controller library to Swiftray
target_link_libraries(Swiftray PRIVATE motion_controller)
