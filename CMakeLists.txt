cmake_minimum_required(VERSION 3.10)
include(ExternalProject)
project(uitk)

# ---- Global configuration ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    # Clang doesn't even warn about forgetting to return a value!
    add_compile_options(-Werror=return-type)
endif()

# ---- UITK ----
add_subdirectory(uitk)  # library
add_subdirectory(tests)
