cmake_minimum_required(VERSION 3.16)

project(linux_system_monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)

if(Qt6_FOUND)
    set(QT_WIDGETS_LIB Qt6::Widgets)
else()
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_WIDGETS_LIB Qt5::Widgets)
endif()

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
    src/options.cpp
    src/gui.cpp
    src/tui.cpp
)

target_compile_options(linux_mon_main PRIVATE
    -Wall
    -Wextra
    -g
)

target_link_libraries(linux_mon_main PRIVATE
    ${QT_WIDGETS_LIB}
    ncurses
)