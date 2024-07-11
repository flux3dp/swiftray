set(SRC
	lcsExpr.cpp
)
set(HEADERS
    lcsExpr.h
)

set(LIBRARY
	lib
)

add_library(liblcs ${SRC})

if(NOT WIN32)
    target_link_libraries(liblcs PRIVATE dl)
endif()