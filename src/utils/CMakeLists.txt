target_sources(Swiftray PRIVATE
  software_update.cpp
)
if (ENABLE_SPARKLE)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_sources(Swiftray PRIVATE
      sparkle_bridge.m
    )
  endif()
endif ()
