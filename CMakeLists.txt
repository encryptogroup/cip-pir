cmake_minimum_required(VERSION 3.14)
project(CIP_PIR)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-march=haswell -mavx512f")

set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -O3")

option(USE_CUDA "USE CUDA" ON)

if(USE_CUDA)
   enable_language("CUDA")
    #set(CMAKE_CUDA_COMPILER /opt/cuda/bin/nvcc)
endif()

if (NOT_CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()
if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

find_package(ENCRYPTO_utils QUIET)
if(ENCRYPTO_utils_FOUND)
    message(STATUS "Found ENCRYPTO_utils")
elseif(NOT ENCRYPTO_utils_FOUND AND NOT TARGET ENCRYPTO_utils::encrypto_utils)
    message("ENCRYPTO_utils was not found: adding ENCRYPTO_utils subdirectory")
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/ENCRYPTO_utils/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initializing Git submodule: extern/ENCRYPTO_utils")
        execute_process(COMMAND git submodule update --init extern/ENCRYPTO_utils
                        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif()
    add_subdirectory(extern/ENCRYPTO_utils)
endif()

find_package(OpenMP QUIET)

add_subdirectory(src)
