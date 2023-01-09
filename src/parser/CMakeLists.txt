add_subdirectory(dxf_rs)
target_sources(Swiftray PRIVATE
  svgpp-parser.cpp
  svgpp-impl.cpp
  svgpp-external-impl.cpp
  dxf_reader.cpp
  my_qsvg_handler.cpp
  qsvggraphics.cpp
  pdf2svg.cpp
)
