#include "monitor.hpp"

#include <filesystem>
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace fs = std::filesystem;

static bool is_number(const std::string& text)
{
    if (text.empty())
    {
        return false;
    }

    for (char ch : text)
    {
        if (!std::isdigit(static_cast<unsigned char>(ch)))
        {
            return false;
        }
    }

    return true;
}

static std::string trim(const std::string& text)
{
    std::size_t start = text.find_first_not_of(" \t");

    if (start == std::string::npos)
    {
        return "";
    }

    std::size_t end = text.find_last_not_of(" \t");

    return text.substr(start, end - start + 1);
}

static bool starts_with(const std::string& text, const std::string& prefix)
{
    return text.rfind(prefix, 0) == 0;
}

static uint64_t parse_kb_value(const std::string& line)
{
    std::size_t colon_position = line.find(':');

    if (colon_position == std::string::npos)
    {
        return 0;
    }

    std::string value_part = line.substr(colon_position + 1);

    std::istringstream iss(value_part);

    uint64_t value = 0;
    iss >> value;

    return value;
}

static uint64_t read_process_cpu_time(int pid)
{
    std::string path = "/proc/" + std::to_string(pid) + "/stat";

    std::ifstream file(path);

    if (!file.is_open())
    {
        return 0;
    }

    std::string line;
    std::getline(file, line);

    /*
        /proc/[pid]/stat имеет сложный формат.

        Внутри есть имя процесса в скобках:
        1234 (firefox) S ...

        Поэтому просто читать через >> опасно.
        Сначала находим закрывающую скобку ')',
        а потом читаем поля после неё.
    */

    std::size_t close_bracket_position = line.rfind(')');

    if (close_bracket_position == std::string::npos)
    {
        return 0;
    }

    std::string after_name = line.substr(close_bracket_position + 2);

    std::istringstream iss(after_name);

    std::vector<std::string> fields;
    std::string field;

    while (iss >> field)
    {
        fields.push_back(field);
    }

    /*
        После имени процесса:
        fields[0]  = state
        fields[1]  = ppid
        ...
        fields[11] = utime
        fields[12] = stime

        utime — время процесса в user mode
        stime — время процесса в kernel mode
    */

    if (fields.size() <= 12)
    {
        return 0;
    }

    uint64_t utime = 0;
    uint64_t stime = 0;

    try
    {
        utime = std::stoull(fields[11]);
        stime = std::stoull(fields[12]);
    }
    catch (...)
    {
        return 0;
    }

    return utime + stime;
}

static ProcessInfo read_single_process(int pid)
{
    ProcessInfo process;
    process.pid = pid;

    std::string path = "/proc/" + std::to_string(pid) + "/status";

    std::ifstream file(path);

    if (!file.is_open())
    {
        throw std::runtime_error("Не удалось открыть " + path);
    }

    std::string line;

    while (std::getline(file, line))
    {
        if (starts_with(line, "Name:"))
        {
            std::size_t colon_position = line.find(':');
            process.name = trim(line.substr(colon_position + 1));
        }
        else if (starts_with(line, "State:"))
        {
            std::size_t colon_position = line.find(':');
            process.state = trim(line.substr(colon_position + 1));
        }
        else if (starts_with(line, "VmRSS:"))
        {
            process.memory_kb = parse_kb_value(line);
        }
    }

    process.cpu_time = read_process_cpu_time(pid);

    return process;
}

std::vector<ProcessInfo> read_processes()
{
    std::vector<ProcessInfo> processes;

    for (const auto& entry : fs::directory_iterator("/proc"))
    {
        try
        {
            std::string filename = entry.path().filename().string();

            if (!is_number(filename))
            {
                continue;
            }

            int pid = std::stoi(filename);

            ProcessInfo process = read_single_process(pid);

            if (!process.name.empty())
            {
                processes.push_back(process);
            }
        }
        catch (...)
        {
            /*
                Это нормально.

                Процесс мог исчезнуть во время чтения.
                Например, мы увидели /proc/5000,
                но пока читали, процесс уже завершился.
            */
            continue;
        }
    }

    return processes;
}

void calculate_process_cpu_usage(
    std::vector<ProcessInfo>& current_processes,
    const std::vector<ProcessInfo>& previous_processes,
    uint64_t previous_total_cpu,
    uint64_t current_total_cpu
)
{
    if (current_total_cpu <= previous_total_cpu)
    {
        return;
    }

    uint64_t total_cpu_difference = current_total_cpu - previous_total_cpu;

    std::unordered_map<int, uint64_t> previous_cpu_by_pid;

    for (const ProcessInfo& process : previous_processes)
    {
        previous_cpu_by_pid[process.pid] = process.cpu_time;
    }

    for (ProcessInfo& current_process : current_processes)
    {
        auto it = previous_cpu_by_pid.find(current_process.pid);

        if (it == previous_cpu_by_pid.end())
        {
            current_process.cpu_usage = 0.0;
            continue;
        }

        uint64_t previous_process_cpu_time = it->second;

        if (current_process.cpu_time < previous_process_cpu_time)
        {
            current_process.cpu_usage = 0.0;
            continue;
        }

        uint64_t process_cpu_difference =
            current_process.cpu_time - previous_process_cpu_time;

        current_process.cpu_usage =
            static_cast<double>(process_cpu_difference) /
            static_cast<double>(total_cpu_difference) *
            100.0;
    }
}

std::vector<ProcessInfo> get_top_processes_by_memory(
    const std::vector<ProcessInfo>& processes,
    std::size_t limit
)
{
    std::vector<ProcessInfo> result = processes;

    std::sort(
        result.begin(),
        result.end(),
        [](const ProcessInfo& a, const ProcessInfo& b)
        {
            return a.memory_kb > b.memory_kb;
        }
    );

    if (result.size() > limit)
    {
        result.resize(limit);
    }

    return result;
}

std::vector<ProcessInfo> get_top_processes_by_cpu(
    const std::vector<ProcessInfo>& processes,
    std::size_t limit
)
{
    std::vector<ProcessInfo> result = processes;

    std::sort(
        result.begin(),
        result.end(),
        [](const ProcessInfo& a, const ProcessInfo& b)
        {
            return a.cpu_usage > b.cpu_usage;
        }
    );

    if (result.size() > limit)
    {
        result.resize(limit);
    }

    return result;
}

double kb_to_mb(uint64_t kb)
{
    return static_cast<double>(kb) / 1024.0;
}