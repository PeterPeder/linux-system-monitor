#include "monitor.hpp"

struct ProgramOptions
{
    int interval_seconds = 1;
    bool show_processes = true;
    bool show_help = false;
};

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

            print_header(
                options.interval_seconds,
                cpu_usage,
                system_info
            );

            print_memory(
                mem,
                memory_usage,
                used_memory_kb
            );

            print_swap(mem);

            print_disks(disks);

            print_networks(current_networks);

            if (options.show_processes)
            {
                print_process_tables(current_processes);
                previous_processes = current_processes;
            }
            else
            {
                print_process_tables_disabled();
            }

            print_footer();

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