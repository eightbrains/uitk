include_directories(AFTER SYSTEM ..
                                 ${CMAKE_BINARY_DIR}/libnativedraw/include
                   )

if (EMSCRIPTEN)
    # Emscripten does not automatically add optimization
    if (CMAKE_MAKE_TYPE MATCHES "RELEASE")
       add_compile_options(-O3)
    endif ()
endif()


set(TEST_WIDGET_HEADERS fractal.h)
set(TEST_WIDGET_SOURCES test-widgets.cpp
                        fractal.cpp)
if (WIN32)
    add_executable(test-widgets WIN32 ${TEST_WIDGET_HEADERS} ${TEST_WIDGET_SOURCES})
else()
    add_executable(test-widgets ${TEST_WIDGET_SOURCES})
endif()
if (EMSCRIPTEN)
    add_custom_command(TARGET test-widgets POST_BUILD
                              COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/tests/test-widgets.html" $<TARGET_FILE_DIR:test-widgets>
                      )
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
