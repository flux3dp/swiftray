target_sources(Swiftray PRIVATE
  dxf_reader.cpp
)

target_sources(Swiftray PRIVATE
  my_qsvg_handler_qt6.cpp
  "mysvg/qsvguse2.cpp"
)

add_library(pdf2svg STATIC)
target_sources(pdf2svg PRIVATE
  pdf2svg.cpp
)
target_link_libraries(pdf2svg PRIVATE
  Qt::Core
  Cairo::Cairo
  Poppler::Poppler
  GLib::GLib
  GLib::GObject
)
target_link_libraries(Swiftray PRIVATE pdf2svg)


# Compile dxf_rs separately
add_subdirectory(dxf_rs)
target_link_libraries(Swiftray PRIVATE dxf_rs)