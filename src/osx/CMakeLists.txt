if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_sources(Swiftray PRIVATE disable-app-nap.mm)
endif()