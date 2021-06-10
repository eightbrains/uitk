include_directories(AFTER SYSTEM ..
                                 ${CMAKE_BINARY_DIR}/libnativedraw/include
                   )

set(TEST_WIDGET_SOURCES test-widgets.cpp)
add_executable(test-widgets ${TEST_WIDGET_SOURCES})
target_link_libraries(test-widgets uitk)
