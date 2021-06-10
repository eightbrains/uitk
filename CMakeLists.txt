cmake_minimum_required(VERSION 3.10)
include(ExternalProject)
project(uitk)

# ---- Global configuration ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- UITK ----
add_subdirectory(uitk)  # library
add_subdirectory(tests)
