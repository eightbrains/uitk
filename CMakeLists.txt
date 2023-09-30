cmake_minimum_required(VERSION 3.10)
include(ExternalProject)
project(uitk)

# ---- Global configuration ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CMD_FOR_EXTERNAL_PROJECT cmake)

if (APPLE)
    # Clang doesn't even warn about forgetting to return a value!
    add_compile_options(-Werror=return-type)
endif()

if (EMSCRIPTEN)
    # Emscripten does not automatically add optimization
    if (CMAKE_MAKE_TYPE MATCHES "RELEASE")
       add_compile_options(-O3)
    endif ()
    if (CMAKE_MAKE_TYPE MATCHES "Debug")
       add_compile_options(-g)
    endif ()

    set(CMAKE_CMD_FOR_EXTERNAL_PROJECT emcmake cmake)

    set(EMSCRIPTEN_LIBS -sUSE_ZLIB=1 -sUSE_LIBPNG=1 -sUSE_LIBJPEG=1 -sUSE_GIFLIB=1)
    add_compile_options(${EMSCRIPTEN_LIBS})
    add_link_options(-sOFFSCREENCANVAS_SUPPORT=1 -sWASM=1 -sASYNCIFY ${EMSCRIPTEN_LIBS} --bind -lembind)

endif()

# ---- UITK ----
add_subdirectory(uitk)  # library
add_subdirectory(tests)
