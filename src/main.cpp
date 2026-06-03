#include "monitor.hpp"

struct ProgramOptions
{
    int interval_seconds = 1;
    bool show_processes = true;
    bool show_help = false;
};

void clear_screen()
{
    std::cout << "\033[2J\033[H";
}

static std::string make_progress_bar(double percent, int width = 20)
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

static void print_help()
{
    std::cout << "Linux System Monitor\n\n";

    std::cout << "Usage:\n";
    std::cout << "  ./linux_mon_main\n";
    std::cout << "  ./linux_mon_main --interval 2\n";
    std::cout << "  ./linux_mon_main --no-processes\n";
    std::cout << "  ./linux_mon_main --help\n\n";

    std::cout << "Options:\n";
    std::cout << "  --help          Show this help message\n";
    std::cout << "  --interval N    Update interval in seconds\n";
    std::cout << "  --no-processes  Hide process tables\n";
}

static ProgramOptions parse_arguments(int argc, char* argv[])
{
    ProgramOptions options;

    for (int i = 1; i < argc; ++i)
    {
        std::string argument = argv[i];

        if (argument == "--help")
        {
            options.show_help = true;
        }
        else if (argument == "--no-processes")
        {
            options.show_processes = false;
        }
        else if (argument == "--interval")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("После --interval нужно указать число секунд");
            }

            std::string value = argv[i + 1];

            try
            {
                options.interval_seconds = std::stoi(value);
            }
            catch (...)
            {
                throw std::runtime_error("Значение --interval должно быть числом");
            }

            if (options.interval_seconds <= 0)
            {
                throw std::runtime_error("Значение --interval должно быть больше 0");
            }

            ++i;
        }
        else
        {
            throw std::runtime_error("Неизвестный аргумент: " + argument);
        }
    }

    return options;
}

int main(int argc, char* argv[])
{
    try
    {
        ProgramOptions options = parse_arguments(argc, argv);

        if (options.show_help)
        {
            print_help();
            return 0;
        }

        CpuTimes previous_cpu = read_cpu_times();

        std::vector<ProcessInfo> previous_processes;

        if (options.show_processes)
        {
            previous_processes = read_processes();
        }

        std::vector<NetworkInfo> previous_networks = read_networks();

        while (true)
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(options.interval_seconds)
            );

            CpuTimes current_cpu = read_cpu_times();

            std::vector<ProcessInfo> current_processes;

            if (options.show_processes)
            {
                current_processes = read_processes();

                calculate_process_cpu_usage(
                    current_processes,
                    previous_processes,
                    get_total_time(previous_cpu),
                    get_total_time(current_cpu)
                );
            }

            std::vector<NetworkInfo> current_networks = read_networks();

            calculate_network_speed(
                current_networks,
                previous_networks,
                static_cast<double>(options.interval_seconds)
            );

            MemInfo mem = read_mem_info();
            SystemInfo system_info = read_system_info();
            std::vector<DiskInfo> disks = read_disks();

            double cpu_usage = calculate_cpu_usage(previous_cpu, current_cpu);
            double memory_usage = calculate_memory_usage(mem);

            uint64_t used_memory_kb = mem.total_kb - mem.available_kb;

            clear_screen();

            std::cout << "===== Linux System Monitor =====\n\n";

            std::cout << std::fixed << std::setprecision(2);

            std::cout << "Update interval: "
                      << options.interval_seconds
                      << " sec\n";

            std::cout << "CPU usage: "
                      << std::setw(6) << cpu_usage
                      << " % "
                      << make_progress_bar(cpu_usage)
                      << '\n';

            std::cout << "Load average: "
                      << system_info.load_average.one_min << " "
                      << system_info.load_average.five_min << " "
                      << system_info.load_average.fifteen_min << '\n';

            std::cout << "Uptime: "
                      << format_uptime(system_info.uptime_seconds)
                      << "\n\n";

            std::cout << "Memory:\n";
            std::cout << "  Total:     " << kb_to_gb(mem.total_kb) << " GB\n";
            std::cout << "  Used:      " << kb_to_gb(used_memory_kb) << " GB\n";
            std::cout << "  Available: " << kb_to_gb(mem.available_kb) << " GB\n";
            std::cout << "  Usage:     "
                      << std::setw(6) << memory_usage
                      << " % "
                      << make_progress_bar(memory_usage)
                      << "\n\n";

            std::cout << "Swap:\n";
            std::cout << "  Total: " << kb_to_gb(mem.swap_total_kb) << " GB\n";
            std::cout << "  Free:  " << kb_to_gb(mem.swap_free_kb) << " GB\n\n";

            std::cout << "Disk usage:\n";

            std::cout << std::left
                      << std::setw(16) << "MOUNT"
                      << std::setw(10) << "FS"
                      << std::setw(12) << "USED GB"
                      << std::setw(12) << "TOTAL GB"
                      << std::setw(10) << "USAGE %"
                      << std::setw(24) << "BAR"
                      << '\n';

            std::cout << "-------------------------------------------------------------------\n";

            for (const DiskInfo& disk : disks)
            {
                std::cout << std::left
                          << std::setw(16) << disk.mount_point
                          << std::setw(10) << disk.filesystem
                          << std::setw(12) << bytes_to_gb(disk.used_bytes)
                          << std::setw(12) << bytes_to_gb(disk.total_bytes)
                          << std::setw(10) << disk.usage_percent
                          << std::setw(24) << make_progress_bar(disk.usage_percent)
                          << '\n';
            }

            std::cout << '\n';

            std::cout << "Network:\n";

            std::cout << std::left
                      << std::setw(16) << "INTERFACE"
                      << std::setw(18) << "DOWNLOAD KB/s"
                      << std::setw(18) << "UPLOAD KB/s"
                      << '\n';

            std::cout << "----------------------------------------------------\n";

            for (const NetworkInfo& network : current_networks)
            {
                std::cout << std::left
                          << std::setw(16) << network.interface_name
                          << std::setw(18) << network.download_kb_s
                          << std::setw(18) << network.upload_kb_s
                          << '\n';
            }

            std::cout << '\n';

            if (options.show_processes)
            {
                std::vector<ProcessInfo> top_cpu_processes =
                    get_top_processes_by_cpu(current_processes, 10);

                std::vector<ProcessInfo> top_memory_processes =
                    get_top_processes_by_memory(current_processes, 10);

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

                previous_processes = current_processes;
            }
            else
            {
                std::cout << "Process tables: disabled\n";
            }

            std::cout << "\nPress Ctrl + C to exit.\n";

            previous_cpu = current_cpu;
            previous_networks = current_networks;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Ошибка: " << error.what() << '\n';
        std::cerr << "Используй ./linux_mon_main --help для справки\n";
        return 1;
    }

    return 0;
}
