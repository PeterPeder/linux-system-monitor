# Linux System Monitor

Linux System Monitor is a simple CLI system monitoring tool written in C++ for Linux.

The program reads system information from the `/proc` filesystem and displays real-time statistics in the terminal.

## Features

- CPU usage monitoring
- RAM and Swap usage monitoring
- System uptime
- Load average
- Disk usage
- Network download and upload speed
- Total process count
- Top processes by CPU usage
- Top processes by RAM usage
- Text progress bars
- Colored terminal output
- Command-line arguments
- CMake build support

## Project structure

linux-system-monitor/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/
│   └── monitor.hpp
├── src/
│   ├── main.cpp
│   ├── CPU.cpp
│   ├── mem.cpp
│   ├── process.cpp
│   ├── system.cpp
│   ├── disk.cpp
│   └── network.cpp
└── build/

Build

Create a build directory:

mkdir -p build
cd build

Run CMake:

cmake ..

Build the project:

cmake --build .

Run the program:

./linux_mon_main
Usage

Default mode:

./linux_mon_main

Set update interval:

./linux_mon_main --interval 2

Run without process tables:

./linux_mon_main --no-processes

Show help:

./linux_mon_main --help

Combine options:

./linux_mon_main --interval 2 --no-processes
Example output
===== Linux System Monitor =====

Update interval: 1 sec
CPU usage:  12.45 % [##------------------]
Load average: 1.22 1.08 0.94
Uptime: 0d 4h 12m 33s

Memory:
  Total:     23.31 GB
  Used:      18.26 GB
  Available: 5.05 GB
  Usage:     78.34 % [###############-----]

Swap:
  Total: 22.55 GB
  Free:  22.55 GB

Disk usage:
MOUNT           FS        USED GB     TOTAL GB    USAGE %     BAR
----------------------------------------------------------------------------
/               ext4      48.20       120.00      40.16       [########------------]

Network:
INTERFACE       DOWNLOAD KB/s     UPLOAD KB/s
----------------------------------------------------
eth0            128.42            31.80

Top processes by CPU:
PID     NAME                  CPU %       RAM MB
------------------------------------------------------
7228    firefox-esr           2.10        1133.26
Data sources

The program uses Linux system files:

/proc/stat
/proc/meminfo
/proc/uptime
/proc/loadavg
/proc/mounts
/proc/net/dev
/proc/[pid]/status
/proc/[pid]/stat
Requirements
Linux
C++17 compiler
CMake
Make

Install required packages on Debian/Kali/Ubuntu:

sudo apt update
sudo apt install build-essential cmake make -y
Notes

This project was created as an educational Linux system programming project.

The goal is to understand how system monitoring tools work internally by reading Linux kernel-provided information from the /proc filesystem.
