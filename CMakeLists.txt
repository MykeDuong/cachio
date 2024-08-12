cmake_minimum_required(VERSION 3.25.2 FATAL_ERROR)

project(cachio)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "-Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "-g")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(COMMON ./src/common.c ./src/command.c)
set(SOURCES ./src/main.c ./src/connection.c ./src/store.c ./src/command.c ./src/request.c ./src/map.c ./src/entry.c ./src/object.c ./src/encoding.c ${COMMON})
set(CLIENT ./src/client.c ${COMMON})

add_executable(cachio ${SOURCES})
add_executable(client ${CLIENT})
