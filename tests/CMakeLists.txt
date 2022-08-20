include_directories(AFTER SYSTEM ..
                                 ${CMAKE_BINARY_DIR}/libnativedraw/include
                   )

set(TEST_WIDGET_SOURCES test-widgets.cpp)
if (WIN32)
    add_executable(test-widgets WIN32 ${TEST_WIDGET_SOURCES})
else()
    add_executable(test-widgets ${TEST_WIDGET_SOURCES})
endif()
target_link_libraries(test-widgets uitk)

set(TEST_SOURCES test-io.cpp)
add_executable(test-io ${TEST_SOURCES})
target_link_libraries(test-io uitk)
