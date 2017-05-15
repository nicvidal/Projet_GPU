cmake_minimum_required(VERSION 2.6)

project(atoms)

# User options
option(USE_DOUBLE "Use double precision" OFF)
if (USE_DOUBLE)
    add_definitions(-DUSE_DOUBLE=1)
else (USE_DOUBLE)
    add_definitions(-DUSE_DOUBLE=0)
endif (USE_DOUBLE)

add_subdirectory(libsotl)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(SOTL_PATH ${CMAKE_SOURCE_DIR}/libsotl)

# Compiler options and source code for the binary.
add_definitions(-std=c99 -g -Wno-deprecated  -Wno-unused-function)
file(GLOB CSources src/*.c)
add_executable(${PROJECT_NAME} ${CSources})

# libsotl.
include_directories(${SOTL_PATH}/include)
target_link_libraries(${PROJECT_NAME} sotl)
