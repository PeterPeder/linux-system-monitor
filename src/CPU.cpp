#include "monitor.hpp"

CpuTimes read_cpu_times()
{
    std::ifstream file("/proc/stat");

    if (!file.is_open())
    {
        throw std::runtime_error("Ошибка открытия /proc/stat");
    }

    std::string line;
    std::getline(file, line);

    std::istringstream iss(line);

    std::string cpu_label;
    CpuTimes times;

    iss >> cpu_label
        >> times.user
        >> times.nice
        >> times.system
        >> times.idle
        >> times.iowait
        >> times.irq
        >> times.softirq
        >> times.steal
        >> times.guest
        >> times.guest_nice;

    return times;
}

uint64_t get_idle_time(const CpuTimes& t)
{
    return t.idle + t.iowait;
}

uint64_t get_total_time(const CpuTimes& t)
{
    return t.user
         + t.nice
         + t.system
         + t.idle
         + t.iowait
         + t.irq
         + t.softirq
         + t.steal;
}

double calculate_cpu_usage(const CpuTimes& previous, const CpuTimes& current)
{
    uint64_t previous_idle = get_idle_time(previous);
    uint64_t current_idle = get_idle_time(current);

    uint64_t previous_total = get_total_time(previous);
    uint64_t current_total = get_total_time(current);

    uint64_t total_difference = current_total - previous_total;
    uint64_t idle_difference = current_idle - previous_idle;

    if (total_difference == 0)
    {
        return 0.0;
    }

    return (1.0 - static_cast<double>(idle_difference) / total_difference) * 100.0;
}