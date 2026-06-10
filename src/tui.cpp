#include "monitor.hpp"

#include <ncurses.h>
#include <algorithm>

static int get_color_pair_by_percent(double percent)
{
    if (percent >= 85.0)
    {
        return 3; // red
    }

    if (percent >= 60.0)
    {
        return 2; // yellow
    }

    return 1; // green
}

static void draw_progress_bar(int y, int x, double percent, int width)
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
    int color_pair = get_color_pair_by_percent(percent);

    mvprintw(y, x, "[");

    attron(COLOR_PAIR(color_pair));

    for (int i = 0; i < width; ++i)
    {
        if (i < filled)
        {
            mvprintw(y, x + 1 + i, "#");
        }
        else
        {
            mvprintw(y, x + 1 + i, "-");
        }
    }

    attroff(COLOR_PAIR(color_pair));

    mvprintw(y, x + width + 1, "]");
}

static void draw_title()
{
    attron(A_BOLD);
    mvprintw(0, 2, "Linux System Monitor TUI");
    attroff(A_BOLD);

    mvprintw(1, 2, "Press q to exit");
}

static void draw_header_info(
    int interval_seconds,
    double cpu_usage,
    const SystemInfo& system_info
)
{
    mvprintw(3, 2, "Update interval: %d sec", interval_seconds);

    mvprintw(5, 2, "CPU usage:");
    mvprintw(5, 15, "%6.2f %%", cpu_usage);
    draw_progress_bar(5, 27, cpu_usage, 30);

    mvprintw(
        6,
        2,
        "Load average: %.2f %.2f %.2f",
        system_info.load_average.one_min,
        system_info.load_average.five_min,
        system_info.load_average.fifteen_min
    );

    std::string uptime = format_uptime(system_info.uptime_seconds);
    mvprintw(7, 2, "Uptime: %s", uptime.c_str());
}

static void draw_memory(
    const MemInfo& mem,
    double memory_usage,
    uint64_t used_memory_kb
)
{
    mvprintw(9, 2, "Memory:");
    mvprintw(10, 4, "Total:     %.2f GB", kb_to_gb(mem.total_kb));
    mvprintw(11, 4, "Used:      %.2f GB", kb_to_gb(used_memory_kb));
    mvprintw(12, 4, "Available: %.2f GB", kb_to_gb(mem.available_kb));

    mvprintw(13, 4, "Usage:     %6.2f %%", memory_usage);
    draw_progress_bar(13, 26, memory_usage, 30);

    mvprintw(15, 2, "Swap:");
    mvprintw(16, 4, "Total: %.2f GB", kb_to_gb(mem.swap_total_kb));
    mvprintw(17, 4, "Free:  %.2f GB", kb_to_gb(mem.swap_free_kb));
}

static void draw_disks(const std::vector<DiskInfo>& disks)
{
    int start_y = 19;

    mvprintw(start_y, 2, "Disk usage:");
    mvprintw(start_y + 1, 2, "Mount           FS        Used GB     Total GB    Usage");

    int row = start_y + 2;

    for (const DiskInfo& disk : disks)
    {
        if (row >= LINES - 15)
        {
            break;
        }

        mvprintw(
            row,
            2,
            "%-15.15s %-9.9s %-11.2f %-11.2f %6.2f %%",
            disk.mount_point.c_str(),
            disk.filesystem.c_str(),
            bytes_to_gb(disk.used_bytes),
            bytes_to_gb(disk.total_bytes),
            disk.usage_percent
        );

        draw_progress_bar(row, 66, disk.usage_percent, 20);

        ++row;
    }
}

static void draw_networks(const std::vector<NetworkInfo>& networks)
{
    int start_y = LINES - 12;

    if (start_y < 5)
    {
        return;
    }

    mvprintw(start_y, 2, "Network:");
    mvprintw(start_y + 1, 2, "Interface        Download KB/s    Upload KB/s");

    int row = start_y + 2;

    for (const NetworkInfo& network : networks)
    {
        if (row >= LINES - 1)
        {
            break;
        }

        mvprintw(
            row,
            2,
            "%-16.16s %-16.2f %-16.2f",
            network.interface_name.c_str(),
            network.download_kb_s,
            network.upload_kb_s
        );

        ++row;
    }
}

static void draw_processes(
    const std::vector<ProcessInfo>& current_processes,
    std::size_t top_process_count
)
{
    if (COLS < 95)
    {
        mvprintw(3, COLS / 2, "Process table hidden: terminal is too narrow");
        return;
    }

    std::vector<ProcessInfo> top_cpu_processes =
        get_top_processes_by_cpu(current_processes, top_process_count);

    int start_x = 95;
    int start_y = 3;

    if (start_x >= COLS)
    {
        return;
    }

    mvprintw(start_y, start_x, "Top processes by CPU:");
    mvprintw(start_y + 1, start_x, "PID      NAME                 CPU %%    RAM MB");

    int row = start_y + 2;

    for (const ProcessInfo& process : top_cpu_processes)
    {
        if (row >= LINES - 1)
        {
            break;
        }

        mvprintw(
            row,
            start_x,
            "%-8d %-20.20s %-8.2f %-8.2f",
            process.pid,
            process.name.c_str(),
            process.cpu_usage,
            kb_to_mb(process.memory_kb)
        );

        ++row;
    }
}

static void initialize_tui()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    if (has_colors())
    {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_GREEN, -1);
        init_pair(2, COLOR_YELLOW, -1);
        init_pair(3, COLOR_RED, -1);
    }
}

void run_tui(const ProgramOptions& options)
{
    initialize_tui();

    try
    {
        CpuTimes previous_cpu = read_cpu_times();

        std::vector<ProcessInfo> previous_processes;

        if (options.show_processes)
        {
            previous_processes = read_processes();
        }

        std::vector<NetworkInfo> previous_networks = read_networks();

        clear();

        mvprintw(0, 2, "Collecting first sample...");
        refresh();

        while (true)
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(options.interval_seconds)
            );

            int key = getch();

            if (key == 'q' || key == 'Q')
            {
                break;
            }

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

            clear();

            if (LINES < 25 || COLS < 80)
            {
                mvprintw(0, 0, "Terminal window is too small.");
                mvprintw(1, 0, "Please resize terminal to at least 80x25.");
                mvprintw(3, 0, "Press q to exit.");
                refresh();

                previous_cpu = current_cpu;
                previous_networks = current_networks;

                if (options.show_processes)
                {
                    previous_processes = current_processes;
                }

                continue;
            }

            draw_title();

            draw_header_info(
                options.interval_seconds,
                cpu_usage,
                system_info
            );

            draw_memory(
                mem,
                memory_usage,
                used_memory_kb
            );

            draw_disks(disks);

            draw_networks(current_networks);

            if (options.show_processes)
            {
                draw_processes(
                    current_processes,
                    options.top_process_count
                );
            }
            else
            {
                mvprintw(3, 95, "Process tables: disabled");
            }

            refresh();

            previous_cpu = current_cpu;
            previous_networks = current_networks;

            if (options.show_processes)
            {
                previous_processes = current_processes;
            }
        }

        endwin();
    }
    catch (...)
    {
        endwin();
        throw;
    }
}