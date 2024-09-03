target_sources(Swiftray PRIVATE
  dxf_reader.cpp
  pdf2svg.cpp
)

target_sources(Swiftray PRIVATE
  my_qsvg_handler_qt6.cpp
  "mysvg/qsvguse2.cpp"
)

# Compile dxf_rs separately
add_subdirectory(dxf_rs)

target_link_libraries(Swiftray PRIVATE dxf_rs)
