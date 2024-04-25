cmake_minimum_required(VERSION 3.15)
project(Quake1)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -Og -fcommon -Wno-error=implicit-function-declaration")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
set(CMAKE_EXE_LINKER_FLAGS "-m32")

set(SDL2_LIBRARIES "/usr/lib32/libSDL2.so" "/usr/lib32/libSDL2main.a")
set(SDL2_INCLUDE_DIRS "/usr/include/SDL2")

include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB SOURCES "src/*.c")

add_executable(Quake ${SOURCES})

target_link_libraries(Quake m c ${SDL2_LIBRARIES})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(test
    COMMAND Quake +timedemo demo1
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
