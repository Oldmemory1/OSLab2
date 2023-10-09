cmake_minimum_required(VERSION 3.26)
project(osLab2 C)

set(CMAKE_C_STANDARD 11)

add_executable(osLab2 main.c
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fexec-charset=GBK")
