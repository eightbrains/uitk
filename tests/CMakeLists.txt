include_directories(AFTER SYSTEM ..
                                 ${CMAKE_BINARY_DIR}/libnativedraw/include
                   )

set(TEST_WIDGET_HEADERS fractal.h)
set(TEST_WIDGET_SOURCES test-widgets.cpp
                        fractal.cpp)
if (WIN32)
    add_executable(test-widgets WIN32 ${TEST_WIDGET_HEADERS} ${TEST_WIDGET_SOURCES})
else()
    add_executable(test-widgets ${TEST_WIDGET_SOURCES})
endif()
target_link_libraries(test-widgets uitk)

set(TEST_IO_HEADERS TestCase.h)
set(TEST_IO_SOURCES test-io.cpp)
add_executable(test-io ${TEST_IO_HEADERS} ${TEST_IO_SOURCES})
target_link_libraries(test-io uitk)

set(TEST_HEADERS TestCase.h)
set(TEST_SOURCES test.cpp)
add_executable(test ${TEST_HEADERS} ${TEST_SOURCES})
target_link_libraries(test uitk)
