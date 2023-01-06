include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/8d51ffdfab10b3fba636ae69bc03da4b54f8c235.zip
)

FetchContent_MakeAvailable(googletest)
enable_testing()

find_package(Threads REQUIRED)

SET(TEST_SOURCE ${SOURCES})
qt5_add_resources(TEST_SOURCE "../qml.qrc")
qt5_add_resources(TEST_SOURCE "../sparkle.qrc")

add_executable(UnitTest main.cpp ${TEST_SOURCE} ${GTestFiles})
target_link_libraries(UnitTest gtest_main)
target_link_libraries(UnitTest Qt::Core)
target_link_libraries(UnitTest Qt::Quick)
target_link_libraries(UnitTest Qt::QuickWidgets)
target_link_libraries(UnitTest Qt::OpenGL)
target_link_libraries(UnitTest Qt::Gui)
target_link_libraries(UnitTest Qt::Svg)
target_link_libraries(UnitTest Qt::Widgets)
target_link_libraries(UnitTest Qt::SerialPort)

add_test(UnitTest UnitTest)