#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <cstddef>

// ================= CPU =================

struct CpuTimes
{
    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
    uint64_t steal = 0;
    uint64_t guest = 0;
    uint64_t guest_nice = 0;
};

CpuTimes read_cpu_times();
uint64_t get_idle_time(const CpuTimes& t);
uint64_t get_total_time(const CpuTimes& t);
double calculate_cpu_usage(const CpuTimes& previous, const CpuTimes& current);

// ================= PROCESSES =================

struct ProcessInfo
{
    int pid = 0;
    std::string name;
    std::string state;
    uint64_t memory_kb = 0;

    uint64_t cpu_time = 0;
    double cpu_usage = 0.0;
};

std::vector<ProcessInfo> read_processes();

std::vector<ProcessInfo> get_top_processes_by_memory(
    const std::vector<ProcessInfo>& processes,
    std::size_t limit
);

std::vector<ProcessInfo> get_top_processes_by_cpu(
    const std::vector<ProcessInfo>& processes,
    std::size_t limit
);

void calculate_process_cpu_usage(
    std::vector<ProcessInfo>& current_processes,
    const std::vector<ProcessInfo>& previous_processes,
    uint64_t previous_total_cpu,
    uint64_t current_total_cpu
);

double kb_to_mb(uint64_t kb);

// ================= MEMORY =================

struct MemInfo
{
    uint64_t total_kb = 0;
    uint64_t free_kb = 0;
    uint64_t available_kb = 0;
    uint64_t buffers_kb = 0;
    uint64_t cached_kb = 0;
    uint64_t swap_total_kb = 0;
    uint64_t swap_free_kb = 0;
};

MemInfo read_mem_info();
double calculate_memory_usage(const MemInfo& mem);
double kb_to_gb(uint64_t kb);

// ================= SYSTEM =================

struct LoadAverage
{
    double one_min = 0.0;
    double five_min = 0.0;
    double fifteen_min = 0.0;
};

struct SystemInfo
{
    double uptime_seconds = 0.0;
    LoadAverage load_average;
};

SystemInfo read_system_info();
std::string format_uptime(double uptime_seconds);

// ================= DISK =================

struct DiskInfo
{
    std::string device;
    std::string mount_point;
    std::string filesystem;

    uint64_t total_bytes = 0;
    uint64_t free_bytes = 0;
    uint64_t available_bytes = 0;
    uint64_t used_bytes = 0;

    double usage_percent = 0.0;
};

std::vector<DiskInfo> read_disks();
double bytes_to_gb(uint64_t bytes);

// ================= NETWORK =================

struct NetworkInfo
{
    std::string interface_name;

    uint64_t rx_bytes = 0;
    uint64_t tx_bytes = 0;

    double download_kb_s = 0.0;
    double upload_kb_s = 0.0;
};

std::vector<NetworkInfo> read_networks();

void calculate_network_speed(
    std::vector<NetworkInfo>& current_networks,
    const std::vector<NetworkInfo>& previous_networks,
    double elapsed_seconds
);

// ================= OTHER =================

void clear_screen();