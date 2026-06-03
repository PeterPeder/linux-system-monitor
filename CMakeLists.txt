cmake_minimum_required(VERSION 3.16)

project(linux_system_monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(linux_mon_main
    src/main.cpp
    src/CPU.cpp
    src/mem.cpp
    src/process.cpp
    src/system.cpp
    src/disk.cpp
    src/network.cpp
    src/ui.cpp
)

target_compile_options(linux_mon_main PRIVATE
    -Wall
    -Wextra
    -g
)