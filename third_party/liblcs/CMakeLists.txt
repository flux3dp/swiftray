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

target_include_directories(liblcs PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)