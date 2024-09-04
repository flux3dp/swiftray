# Add the panels library

add_library(swiftray_panels STATIC)

target_sources(swiftray_panels PRIVATE
  doc-panel.cpp
  font-panel.cpp
  image-panel.cpp
  jogging-panel.cpp
  layer-panel.cpp
  layer-params-panel.cpp
  spooler-panel.cpp
  transform-panel.cpp
  laser-panel.cpp
)

target_link_libraries(swiftray_panels PRIVATE Qt::Core)
target_link_libraries(swiftray_panels PRIVATE Qt::Widgets)
target_link_libraries(swiftray_panels PRIVATE Qt::Gui) 
target_link_libraries(swiftray_panels PRIVATE Qt::SerialPort)
target_link_libraries(swiftray_panels PRIVATE Qt::OpenGLWidgets)
target_link_libraries(swiftray_panels PRIVATE Qt::Quick)
target_link_libraries(swiftray_panels PRIVATE Qt::QuickWidgets)
target_link_libraries(swiftray_panels PRIVATE Qt::QuickControls2)
target_link_libraries(swiftray_panels PRIVATE Qt::Qml)
target_link_libraries(swiftray_panels PRIVATE Qt::WebSockets)

# Link panels library to Swiftray
target_link_libraries(Swiftray PRIVATE swiftray_panels)