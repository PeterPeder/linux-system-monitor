#include "monitor.hpp"

SystemInfo read_system_info()
{
    SystemInfo info;

    std::ifstream uptime_file("/proc/uptime");

    if (!uptime_file.is_open())
    {
        throw std::runtime_error("Ошибка открытия /proc/uptime");
    }

    uptime_file >> info.uptime_seconds;

    std::ifstream loadavg_file("/proc/loadavg");

    if (!loadavg_file.is_open())
    {
        throw std::runtime_error("Ошибка открытия /proc/loadavg");
    }

    loadavg_file
        >> info.load_average.one_min
        >> info.load_average.five_min
        >> info.load_average.fifteen_min;

    return info;
}

std::string format_uptime(double uptime_seconds)
{
    uint64_t total_seconds = static_cast<uint64_t>(uptime_seconds);

    uint64_t days = total_seconds / 86400;
    total_seconds %= 86400;

    uint64_t hours = total_seconds / 3600;
    total_seconds %= 3600;

    uint64_t minutes = total_seconds / 60;
    uint64_t seconds = total_seconds % 60;

    std::ostringstream oss;

    oss << days << "d "
        << hours << "h "
        << minutes << "m "
        << seconds << "s";

    return oss.str();
}