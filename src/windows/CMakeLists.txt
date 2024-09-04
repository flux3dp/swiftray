# Create windows library

add_library(swiftray_windows STATIC)

target_sources(swiftray_windows PRIVATE
  mainwindow.cpp
  gcode-panel.cpp
  about-window.cpp
  image-crop-dialog.cpp
  image-sharpen-dialog.cpp
  image-trace-dialog.cpp
  job-dashboard-dialog.cpp
  machine-manager.cpp
  machine-monitor.cpp
  path-offset-dialog.cpp
  preferences-window.cpp
  preset-manager.cpp
  preview-window.cpp
  privacy_window.cpp
  welcome-dialog.cpp
  rotary_setup.cpp
  consoledialog.cpp
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_sources(swiftray_windows PRIVATE osxwindow.mm)
endif()

target_link_libraries(swiftray_windows PRIVATE Qt::Core)
target_link_libraries(swiftray_windows PRIVATE Qt::Widgets)
target_link_libraries(swiftray_windows PRIVATE Qt::Gui) 
target_link_libraries(swiftray_windows PRIVATE Qt::SerialPort)
target_link_libraries(swiftray_windows PRIVATE Qt::OpenGLWidgets)
target_link_libraries(swiftray_windows PRIVATE Qt::Quick)
target_link_libraries(swiftray_windows PRIVATE Qt::QuickWidgets)
target_link_libraries(swiftray_windows PRIVATE Qt::QuickControls2)
target_link_libraries(swiftray_windows PRIVATE Qt::Qml)
target_link_libraries(swiftray_windows PRIVATE Qt::WebSockets)

# Link windows library to Swiftray
target_link_libraries(Swiftray PRIVATE swiftray_windows)