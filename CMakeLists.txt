cmake_minimum_required(VERSION 3.16.3)
project(spawn_server C)

set(CMAKE_C_STANDARD 11)

add_executable(server server/main.c)
add_executable(client client/main.c)
