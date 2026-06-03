#include "monitor.hpp"

MemInfo read_mem_info()
{
    std::ifstream file("/proc/meminfo");

    if (!file.is_open())
    {
        throw std::runtime_error("Ошибка открытия /proc/meminfo");
    }

    MemInfo mem;

    std::string key;
    uint64_t value;
    std::string unit;

    while (file >> key >> value >> unit)
    {
        if (key == "MemTotal:")
        {
            mem.total_kb = value;
        }
        else if (key == "MemFree:")
        {
            mem.free_kb = value;
        }
        else if (key == "MemAvailable:")
        {
            mem.available_kb = value;
        }
        else if (key == "Buffers:")
        {
            mem.buffers_kb = value;
        }
        else if (key == "Cached:")
        {
            mem.cached_kb = value;
        }
        else if (key == "SwapTotal:")
        {
            mem.swap_total_kb = value;
        }
        else if (key == "SwapFree:")
        {
            mem.swap_free_kb = value;
        }
    }

    return mem;
}

double calculate_memory_usage(const MemInfo& mem)
{
    if (mem.total_kb == 0)
    {
        return 0.0;
    }

    uint64_t used_kb = mem.total_kb - mem.available_kb;

    return static_cast<double>(used_kb) / mem.total_kb * 100.0;
}

double kb_to_gb(uint64_t kb)
{
    return static_cast<double>(kb) / 1024.0 / 1024.0;
}