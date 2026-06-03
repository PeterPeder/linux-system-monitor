#include "monitor.hpp"

#include <unordered_map>
#include <algorithm>

static std::string trim_network_text(const std::string& text)
{
    std::size_t start = text.find_first_not_of(" \t");

    if (start == std::string::npos)
    {
        return "";
    }

    std::size_t end = text.find_last_not_of(" \t");

    return text.substr(start, end - start + 1);
}

std::vector<NetworkInfo> read_networks()
{
    std::vector<NetworkInfo> networks;

    std::ifstream file("/proc/net/dev");

    if (!file.is_open())
    {
        throw std::runtime_error("Ошибка открытия /proc/net/dev");
    }

    std::string line;

    // Пропускаем первые две строки заголовка
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::size_t colon_position = line.find(':');

        if (colon_position == std::string::npos)
        {
            continue;
        }

        std::string interface_name = line.substr(0, colon_position);
        interface_name = trim_network_text(interface_name);

        std::string data_part = line.substr(colon_position + 1);
        std::istringstream iss(data_part);

        NetworkInfo network;
        network.interface_name = interface_name;

        uint64_t rx_packets = 0;
        uint64_t rx_errors = 0;
        uint64_t rx_drop = 0;
        uint64_t rx_fifo = 0;
        uint64_t rx_frame = 0;
        uint64_t rx_compressed = 0;
        uint64_t rx_multicast = 0;

        uint64_t tx_packets = 0;
        uint64_t tx_errors = 0;
        uint64_t tx_drop = 0;
        uint64_t tx_fifo = 0;
        uint64_t tx_colls = 0;
        uint64_t tx_carrier = 0;
        uint64_t tx_compressed = 0;

        /*
            /proc/net/dev:

            receive:
            bytes packets errs drop fifo frame compressed multicast

            transmit:
            bytes packets errs drop fifo colls carrier compressed
        */

        iss >> network.rx_bytes
            >> rx_packets
            >> rx_errors
            >> rx_drop
            >> rx_fifo
            >> rx_frame
            >> rx_compressed
            >> rx_multicast
            >> network.tx_bytes
            >> tx_packets
            >> tx_errors
            >> tx_drop
            >> tx_fifo
            >> tx_colls
            >> tx_carrier
            >> tx_compressed;

        if (!network.interface_name.empty())
        {
            networks.push_back(network);
        }
    }

    std::sort(
        networks.begin(),
        networks.end(),
        [](const NetworkInfo& a, const NetworkInfo& b)
        {
            return a.interface_name < b.interface_name;
        }
    );

    return networks;
}

void calculate_network_speed(
    std::vector<NetworkInfo>& current_networks,
    const std::vector<NetworkInfo>& previous_networks,
    double elapsed_seconds
)
{
    if (elapsed_seconds <= 0.0)
    {
        return;
    }

    std::unordered_map<std::string, NetworkInfo> previous_by_name;

    for (const NetworkInfo& network : previous_networks)
    {
        previous_by_name[network.interface_name] = network;
    }

    for (NetworkInfo& current_network : current_networks)
    {
        auto it = previous_by_name.find(current_network.interface_name);

        if (it == previous_by_name.end())
        {
            current_network.download_kb_s = 0.0;
            current_network.upload_kb_s = 0.0;
            continue;
        }

        const NetworkInfo& previous_network = it->second;

        if (current_network.rx_bytes >= previous_network.rx_bytes)
        {
            uint64_t rx_difference =
                current_network.rx_bytes - previous_network.rx_bytes;

            current_network.download_kb_s =
                static_cast<double>(rx_difference) / 1024.0 / elapsed_seconds;
        }

        if (current_network.tx_bytes >= previous_network.tx_bytes)
        {
            uint64_t tx_difference =
                current_network.tx_bytes - previous_network.tx_bytes;

            current_network.upload_kb_s =
                static_cast<double>(tx_difference) / 1024.0 / elapsed_seconds;
        }
    }
}