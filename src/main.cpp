#include "monitor.hpp"

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

        if (options.use_tui)
        {
            run_tui(options);
            return 0;
        }

        if (options.use_gui)
        {
            run_gui(options);
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
                print_process_tables(
                    current_processes,
                    options.top_process_count
                );
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