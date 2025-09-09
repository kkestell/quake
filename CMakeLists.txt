cmake_minimum_required(VERSION 3.10)
project(quake C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Find raylib library and headers directly
find_path(RAYLIB_INCLUDE_DIR NAMES raylib.h)
find_library(RAYLIB_LIBRARY NAMES raylib)

file(GLOB SOURCES "src/*.c")

add_executable(quake ${SOURCES})

target_compile_options(quake PRIVATE
        -O3
        -g3
        -fno-omit-frame-pointer
        -Wno-implicit-function-declaration
        -fcommon
        -ffast-math
)

# Use the variables from find_path and find_library
target_include_directories(quake PRIVATE ${RAYLIB_INCLUDE_DIR})
target_link_libraries(quake PRIVATE ${RAYLIB_LIBRARY} m)

add_custom_command(
        TARGET quake POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/id1"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/id1"
)

add_custom_target(test
        COMMAND quake +timedemo demo1
        DEPENDS quake
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
