add_library(beamgo STATIC
  protocol.cpp
  compiler.cpp
  transport.cpp
  controller.cpp
  manager.cpp
)
set_target_properties(beamgo PROPERTIES AUTOMOC ON)
target_compile_definitions(beamgo PRIVATE QT_NO_KEYWORDS)
target_include_directories(beamgo PUBLIC ..)
target_link_libraries(beamgo PUBLIC Qt6::Core Qt6::SerialPort)
if(WIN32)
  target_link_libraries(beamgo PRIVATE bcrypt)
elseif(NOT APPLE)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(beamgo PRIVATE OpenSSL::Crypto)
endif()
