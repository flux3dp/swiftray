add_subdirectory(dxf_rs)
target_sources(Swiftray PRIVATE
  dxf_reader.cpp
  pdf2svg.cpp
)

target_sources(Swiftray PRIVATE
  my_qsvg_handler_qt6.cpp
  "mysvg/qsvguse2.cpp"
)