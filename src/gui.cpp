#include "monitor.hpp"

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
#include <QMessageBox>
#include <QString>

#include <algorithm>

static int percent_to_int(double percent)
{
    if (percent < 0.0)
    {
        percent = 0.0;
    }

    if (percent > 100.0)
    {
        percent = 100.0;
    }

    return static_cast<int>(percent);
}

static QString format_double(double value)
{
    return QString::number(value, 'f', 2);
}

static void fill_disk_table(
    QTableWidget* table,
    const std::vector<DiskInfo>& disks
)
{
    table->setRowCount(static_cast<int>(disks.size()));

    for (int row = 0; row < static_cast<int>(disks.size()); ++row)
    {
        const DiskInfo& disk = disks[row];

        table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(disk.mount_point)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(disk.filesystem)));
        table->setItem(row, 2, new QTableWidgetItem(format_double(bytes_to_gb(disk.used_bytes))));
        table->setItem(row, 3, new QTableWidgetItem(format_double(bytes_to_gb(disk.total_bytes))));
        table->setItem(row, 4, new QTableWidgetItem(format_double(disk.usage_percent) + " %"));
    }
}

static void fill_network_table(
    QTableWidget* table,
    const std::vector<NetworkInfo>& networks
)
{
    table->setRowCount(static_cast<int>(networks.size()));

    for (int row = 0; row < static_cast<int>(networks.size()); ++row)
    {
        const NetworkInfo& network = networks[row];

        table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(network.interface_name)));
        table->setItem(row, 1, new QTableWidgetItem(format_double(network.download_kb_s)));
        table->setItem(row, 2, new QTableWidgetItem(format_double(network.upload_kb_s)));
    }
}

static void fill_process_table(
    QTableWidget* table,
    const std::vector<ProcessInfo>& processes,
    std::size_t top_process_count
)
{
    std::vector<ProcessInfo> top_processes =
        get_top_processes_by_cpu(processes, top_process_count);

    table->setRowCount(static_cast<int>(top_processes.size()));

    for (int row = 0; row < static_cast<int>(top_processes.size()); ++row)
    {
        const ProcessInfo& process = top_processes[row];

        table->setItem(row, 0, new QTableWidgetItem(QString::number(process.pid)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(process.name)));
        table->setItem(row, 2, new QTableWidgetItem(format_double(process.cpu_usage)));
        table->setItem(row, 3, new QTableWidgetItem(format_double(kb_to_mb(process.memory_kb))));
    }
}

void run_gui(const ProgramOptions& options)
{
    int argc = 1;
    char app_name[] = "linux_mon_main";
    char* argv[] = { app_name, nullptr };

    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Linux System Monitor GUI");
    window.resize(1100, 750);

    QVBoxLayout* main_layout = new QVBoxLayout(&window);

    QLabel* title_label = new QLabel("Linux System Monitor");
    title_label->setStyleSheet("font-size: 24px; font-weight: bold;");
    main_layout->addWidget(title_label);

    QLabel* status_label = new QLabel("Collecting first sample...");
    main_layout->addWidget(status_label);

    QGroupBox* system_group = new QGroupBox("System");
    QVBoxLayout* system_layout = new QVBoxLayout(system_group);

    QLabel* interval_label = new QLabel();
    QLabel* cpu_label = new QLabel();
    QProgressBar* cpu_bar = new QProgressBar();

    QLabel* load_label = new QLabel();
    QLabel* uptime_label = new QLabel();

    cpu_bar->setRange(0, 100);

    system_layout->addWidget(interval_label);
    system_layout->addWidget(cpu_label);
    system_layout->addWidget(cpu_bar);
    system_layout->addWidget(load_label);
    system_layout->addWidget(uptime_label);

    main_layout->addWidget(system_group);

    QHBoxLayout* middle_layout = new QHBoxLayout();

    QGroupBox* memory_group = new QGroupBox("Memory");
    QVBoxLayout* memory_layout = new QVBoxLayout(memory_group);

    QLabel* memory_total_label = new QLabel();
    QLabel* memory_used_label = new QLabel();
    QLabel* memory_available_label = new QLabel();
    QLabel* memory_usage_label = new QLabel();
    QProgressBar* memory_bar = new QProgressBar();

    memory_bar->setRange(0, 100);

    memory_layout->addWidget(memory_total_label);
    memory_layout->addWidget(memory_used_label);
    memory_layout->addWidget(memory_available_label);
    memory_layout->addWidget(memory_usage_label);
    memory_layout->addWidget(memory_bar);

    QGroupBox* swap_group = new QGroupBox("Swap");
    QVBoxLayout* swap_layout = new QVBoxLayout(swap_group);

    QLabel* swap_total_label = new QLabel();
    QLabel* swap_free_label = new QLabel();

    swap_layout->addWidget(swap_total_label);
    swap_layout->addWidget(swap_free_label);

    middle_layout->addWidget(memory_group);
    middle_layout->addWidget(swap_group);

    main_layout->addLayout(middle_layout);

    QGroupBox* disk_group = new QGroupBox("Disk usage");
    QVBoxLayout* disk_layout = new QVBoxLayout(disk_group);

    QTableWidget* disk_table = new QTableWidget();
    disk_table->setColumnCount(5);
    disk_table->setHorizontalHeaderLabels(
        { "Mount", "FS", "Used GB", "Total GB", "Usage" }
    );
    disk_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    disk_layout->addWidget(disk_table);
    main_layout->addWidget(disk_group);

    QGroupBox* network_group = new QGroupBox("Network");
    QVBoxLayout* network_layout = new QVBoxLayout(network_group);

    QTableWidget* network_table = new QTableWidget();
    network_table->setColumnCount(3);
    network_table->setHorizontalHeaderLabels(
        { "Interface", "Download KB/s", "Upload KB/s" }
    );
    network_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    network_layout->addWidget(network_table);
    main_layout->addWidget(network_group);

    QGroupBox* process_group = new QGroupBox("Top processes by CPU");
    QVBoxLayout* process_layout = new QVBoxLayout(process_group);

    QTableWidget* process_table = new QTableWidget();
    process_table->setColumnCount(4);
    process_table->setHorizontalHeaderLabels(
        { "PID", "Name", "CPU %", "RAM MB" }
    );
    process_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    process_layout->addWidget(process_table);
    main_layout->addWidget(process_group);

    if (!options.show_processes)
    {
        process_group->setTitle("Processes disabled");
        process_table->setEnabled(false);
    }

    CpuTimes previous_cpu = read_cpu_times();

    std::vector<ProcessInfo> previous_processes;

    if (options.show_processes)
    {
        previous_processes = read_processes();
    }

    std::vector<NetworkInfo> previous_networks = read_networks();

    QTimer timer;

    QObject::connect(
        &timer,
        &QTimer::timeout,
        [&]()
        {
            try
            {
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

                status_label->setText("Running");

                interval_label->setText(
                    "Update interval: " +
                    QString::number(options.interval_seconds) +
                    " sec"
                );

                cpu_label->setText(
                    "CPU usage: " +
                    format_double(cpu_usage) +
                    " %"
                );

                cpu_bar->setValue(percent_to_int(cpu_usage));

                load_label->setText(
                    "Load average: " +
                    format_double(system_info.load_average.one_min) +
                    " " +
                    format_double(system_info.load_average.five_min) +
                    " " +
                    format_double(system_info.load_average.fifteen_min)
                );

                uptime_label->setText(
                    "Uptime: " +
                    QString::fromStdString(format_uptime(system_info.uptime_seconds))
                );

                memory_total_label->setText(
                    "Total: " + format_double(kb_to_gb(mem.total_kb)) + " GB"
                );

                memory_used_label->setText(
                    "Used: " + format_double(kb_to_gb(used_memory_kb)) + " GB"
                );

                memory_available_label->setText(
                    "Available: " + format_double(kb_to_gb(mem.available_kb)) + " GB"
                );

                memory_usage_label->setText(
                    "Usage: " + format_double(memory_usage) + " %"
                );

                memory_bar->setValue(percent_to_int(memory_usage));

                swap_total_label->setText(
                    "Total: " + format_double(kb_to_gb(mem.swap_total_kb)) + " GB"
                );

                swap_free_label->setText(
                    "Free: " + format_double(kb_to_gb(mem.swap_free_kb)) + " GB"
                );

                fill_disk_table(disk_table, disks);
                fill_network_table(network_table, current_networks);

                if (options.show_processes)
                {
                    fill_process_table(
                        process_table,
                        current_processes,
                        options.top_process_count
                    );

                    previous_processes = current_processes;
                }

                previous_cpu = current_cpu;
                previous_networks = current_networks;
            }
            catch (const std::exception& error)
            {
                status_label->setText(
                    "Error: " + QString::fromStdString(error.what())
                );
            }
        }
    );

    timer.start(options.interval_seconds * 1000);

    window.show();

    app.exec();
}