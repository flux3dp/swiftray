target_sources(Swiftray PRIVATE
  software_update.c
)
if (ENABLE_SPARKLE)
target_sources(Swiftray PRIVATE
  sparkle_bridge.m
)
endif ()
