include_directories(AFTER SYSTEM ..
                                 ${CMAKE_BINARY_DIR}/libnativedraw/include
                   )

if (EMSCRIPTEN)
    # Emscripten does not automatically add optimization
    if (CMAKE_MAKE_TYPE MATCHES "RELEASE")
       add_compile_options(-O3)
    endif ()
endif()

#-------------------------------------------------------------------------------
set(COLORPICKER_HEADERS "")
set(COLORPICKER_SOURCES colorpicker.cpp)

if (WIN32)
    add_executable(colorpicker WIN32 ${COLORPICKER_HEADERS} ${COLORPICKER_SOURCES})
else()
    add_executable(colorpicker ${COLORPICKER_SOURCES})
endif()
target_link_libraries(colorpicker uitk)
