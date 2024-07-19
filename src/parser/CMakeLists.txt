add_subdirectory(dxf_rs)
target_sources(Swiftray PRIVATE
  dxf_reader.cpp
  pdf2svg.cpp
)

if (${qtver} MATCHES "6")
target_sources(Swiftray PRIVATE
  my_qsvg_handler_qt6.cpp
  qsvggraphics_qt6.cpp
)
else()
target_sources(Swiftray PRIVATE
  svgpp-parser.cpp
  svgpp-impl.cpp
  svgpp-external-impl.cpp
  qsvggraphics.cpp
  my_qsvg_handler.cpp
)
endif()