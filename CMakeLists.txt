cmake_minimum_required(VERSION 3.9)
project(untitled4 C)

set(CMAKE_C_STANDARD 99)

add_executable(untitled4 threadpool.c threadpool.h server.c main.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")