#include "monitor.hpp"

void clear_screen()
{
    std::cout << "\033[2J\033[H";
}

std::string make_progress_bar(double percent, int width)
{
    if (percent < 0.0)
    {
        percent = 0.0;
    }

    if (percent > 100.0)
    {
        percent = 100.0;
    }

    int filled = static_cast<int>((percent / 100.0) * width);

    std::string bar = "[";

    for (int i = 0; i < width; ++i)
    {
        if (i < filled)
        {
            bar += "#";
        }
        else
        {
            bar += "-";
        }
    }

    bar += "]";

    return bar;
}

static std::string color_by_percent(double percent)
{
    if (percent >= 85.0)
    {
        return "\033[31m"; // red
    }

    if (percent >= 60.0)
    {
        return "\033[33m"; // yellow
    }

    return "\033[32m"; // green
}

static std::string reset_color()
{
    return "\033[0m";
}

std::string make_colored_progress_bar(double percent, int width)
{
    return color_by_percent(percent)
         + make_progress_bar(percent, width)
         + reset_color();
}

std::string colorize_percent(double percent)
{
    std::ostringstream oss;

    oss << color_by_percent(percent)
        << std::fixed << std::setprecision(2)
        << percent
        << reset_color();

    return oss.str();
}

void print_header(
    int interval_seconds,
    double cpu_usage,
    const SystemInfo& system_info
)
{
    std::cout << "===== Linux System Monitor =====\n\n";

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Update interval: "
              << interval_seconds
              << " sec\n";

    std::cout << "CPU usage: "
              << std::right << std::setw(6)
              << colorize_percent(cpu_usage)
              << " % "
              << make_colored_progress_bar(cpu_usage)
              << '\n';

    std::cout << "Load average: "
              << system_info.load_average.one_min << " "
              << system_info.load_average.five_min << " "
              << system_info.load_average.fifteen_min << '\n';

    std::cout << "Uptime: "
              << format_uptime(system_info.uptime_seconds)
              << "\n\n";
}

void print_memory(
    const MemInfo& mem,
    double memory_usage,
    uint64_t used_memory_kb
)
{
    std::cout << "Memory:\n";
    std::cout << "  Total:     " << kb_to_gb(mem.total_kb) << " GB\n";
    std::cout << "  Used:      " << kb_to_gb(used_memory_kb) << " GB\n";
    std::cout << "  Available: " << kb_to_gb(mem.available_kb) << " GB\n";

    std::cout << "  Usage:     "
              << std::right << std::setw(6)
              << colorize_percent(memory_usage)
              << " % "
              << make_colored_progress_bar(memory_usage)
              << "\n\n";
}

void print_swap(const MemInfo& mem)
{
    std::cout << "Swap:\n";
    std::cout << "  Total: " << kb_to_gb(mem.swap_total_kb) << " GB\n";
    std::cout << "  Free:  " << kb_to_gb(mem.swap_free_kb) << " GB\n\n";
}

void print_disks(const std::vector<DiskInfo>& disks)
{
    std::cout << "Disk usage:\n";

    std::cout << std::left
              << std::setw(16) << "MOUNT"
              << std::setw(10) << "FS"
              << std::setw(12) << "USED GB"
              << std::setw(12) << "TOTAL GB"
              << std::setw(12) << "USAGE %"
              << "BAR"
              << '\n';

    std::cout << "----------------------------------------------------------------------------\n";

    for (const DiskInfo& disk : disks)
    {
        std::cout << std::left
                  << std::setw(16) << disk.mount_point
                  << std::setw(10) << disk.filesystem
                  << std::setw(12) << bytes_to_gb(disk.used_bytes)
                  << std::setw(12) << bytes_to_gb(disk.total_bytes)
                  << std::setw(12) << colorize_percent(disk.usage_percent)
                  << make_colored_progress_bar(disk.usage_percent)
                  << '\n';
    }

    std::cout << '\n';
}

void print_networks(const std::vector<NetworkInfo>& networks)
{
    std::cout << "Network:\n";

    std::cout << std::left
              << std::setw(16) << "INTERFACE"
              << std::setw(18) << "DOWNLOAD KB/s"
              << std::setw(18) << "UPLOAD KB/s"
              << '\n';

    std::cout << "----------------------------------------------------\n";

    for (const NetworkInfo& network : networks)
    {
        std::cout << std::left
                  << std::setw(16) << network.interface_name
                  << std::setw(18) << network.download_kb_s
                  << std::setw(18) << network.upload_kb_s
                  << '\n';
    }

    std::cout << '\n';
}

void print_process_tables(
    const std::vector<ProcessInfo>& current_processes,
    std::size_t top_process_count
)
{
    std::vector<ProcessInfo> top_cpu_processes =
        get_top_processes_by_cpu(current_processes, top_process_count);

    std::vector<ProcessInfo> top_memory_processes =
        get_top_processes_by_memory(current_processes, top_process_count);

    std::cout << "Total processes: "
              << current_processes.size()
              << "\n\n";

    std::cout << "Top processes by CPU:\n";

    std::cout << std::left
              << std::setw(8)  << "PID"
              << std::setw(22) << "NAME"
              << std::setw(12) << "CPU %"
              << std::setw(12) << "RAM MB"
              << '\n';

    std::cout << "------------------------------------------------------\n";

    for (const ProcessInfo& process : top_cpu_processes)
    {
        std::cout << std::left
                  << std::setw(8)  << process.pid
                  << std::setw(22) << process.name
                  << std::setw(12) << process.cpu_usage
                  << std::setw(12) << kb_to_mb(process.memory_kb)
                  << '\n';
    }

    std::cout << "\nTop processes by RAM:\n";

    std::cout << std::left
              << std::setw(8)  << "PID"
              << std::setw(22) << "NAME"
              << std::setw(12) << "CPU %"
              << std::setw(12) << "RAM MB"
              << '\n';

    std::cout << "------------------------------------------------------\n";

    for (const ProcessInfo& process : top_memory_processes)
    {
        std::cout << std::left
                  << std::setw(8)  << process.pid
                  << std::setw(22) << process.name
                  << std::setw(12) << process.cpu_usage
                  << std::setw(12) << kb_to_mb(process.memory_kb)
                  << '\n';
    }
}

void print_process_tables_disabled()
{
    std::cout << "Process tables: disabled\n";
}

void print_footer()
{
    std::cout << "\nPress Ctrl + C to exit.\n";
}