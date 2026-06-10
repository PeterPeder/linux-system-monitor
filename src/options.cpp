#include "monitor.hpp"

void print_help()
{
    std::cout << "Linux System Monitor\n\n";

    std::cout << "Usage:\n";
    std::cout << "  ./linux_mon_main\n";
    std::cout << "  ./linux_mon_main --tui\n";
    std::cout << "  ./linux_mon_main --gui\n";
    std::cout << "  ./linux_mon_main --interval 2\n";
    std::cout << "  ./linux_mon_main --top 5\n";
    std::cout << "  ./linux_mon_main --no-processes\n";
    std::cout << "  ./linux_mon_main --help\n\n";

    std::cout << "Options:\n";
    std::cout << "  --help          Show this help message\n";
    std::cout << "  --tui           Run terminal graphical interface\n";
    std::cout << "  --gui           Run desktop graphical interface\n";
    std::cout << "  --interval N    Update interval in seconds\n";
    std::cout << "  --top N         Number of processes in top tables\n";
    std::cout << "  --no-processes  Hide process tables\n";
}

ProgramOptions parse_arguments(int argc, char* argv[])
{
    ProgramOptions options;

    for (int i = 1; i < argc; ++i)
    {
        std::string argument = argv[i];

        if (argument == "--help")
        {
            options.show_help = true;
        }
        else if (argument == "--tui")
        {
            options.use_tui = true;
        }
        else if (argument == "--gui")
        {
            options.use_gui = true;
        }
        else if (argument == "--no-processes")
        {
            options.show_processes = false;
        }
        else if (argument == "--top")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("После --top нужно указать количество процессов");
            }

            std::string value = argv[i + 1];

            int top_count = 0;

            try
            {
                top_count = std::stoi(value);
            }
            catch (...)
            {
                throw std::runtime_error("Значение --top должно быть числом");
            }

            if (top_count <= 0)
            {
                throw std::runtime_error("Значение --top должно быть больше 0");
            }

            if (top_count > 100)
            {
                throw std::runtime_error("Значение --top не должно быть больше 100");
            }

            options.top_process_count = static_cast<std::size_t>(top_count);

            ++i;
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