cmake_minimum_required(VERSION 3.12)
project(LSP_PD8 C)

set(CMAKE_C_STANDARD 90)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_executable(LSP_PD8 main.c)