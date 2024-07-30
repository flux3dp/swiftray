find_package(Qt${qtver} COMPONENTS Core REQUIRED)

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
    ${Qt${qtver}Core_INCLUDE_DIRS}
)

if(NOT WIN32)
    target_link_libraries(liblcs PRIVATE dl Qt${qtver}::Core)
endif()