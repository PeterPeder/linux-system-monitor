#include "monitor.hpp"

#include <sys/statvfs.h>
#include <set>
#include <algorithm>

static bool is_real_filesystem(const std::string& filesystem)
{
    static const std::vector<std::string> real_filesystems =
    {
        "ext2",
        "ext3",
        "ext4",
        "xfs",
        "btrfs",
        "f2fs",
        "zfs",
        "vfat",
        "exfat",
        "ntfs",
        "ntfs3",
        "fuseblk"
    };

    return std::find(
        real_filesystems.begin(),
        real_filesystems.end(),
        filesystem
    ) != real_filesystems.end();
}

static std::string decode_mount_path(const std::string& path)
{
    std::string result = path;

    std::size_t position = 0;

    while ((position = result.find("\\040", position)) != std::string::npos)
    {
        result.replace(position, 4, " ");
        position += 1;
    }

    return result;
}

static DiskInfo read_single_disk(
    const std::string& device,
    const std::string& mount_point,
    const std::string& filesystem
)
{
    struct statvfs stats;

    if (statvfs(mount_point.c_str(), &stats) != 0)
    {
        throw std::runtime_error("Не удалось получить информацию о диске: " + mount_point);
    }

    DiskInfo disk;

    disk.device = device;
    disk.mount_point = mount_point;
    disk.filesystem = filesystem;

    uint64_t block_size = static_cast<uint64_t>(stats.f_frsize);

    disk.total_bytes = static_cast<uint64_t>(stats.f_blocks) * block_size;
    disk.free_bytes = static_cast<uint64_t>(stats.f_bfree) * block_size;
    disk.available_bytes = static_cast<uint64_t>(stats.f_bavail) * block_size;

    if (disk.total_bytes >= disk.free_bytes)
    {
        disk.used_bytes = disk.total_bytes - disk.free_bytes;
    }

    if (disk.total_bytes > 0)
    {
        disk.usage_percent =
            static_cast<double>(disk.used_bytes) /
            static_cast<double>(disk.total_bytes) *
            100.0;
    }

    return disk;
}

std::vector<DiskInfo> read_disks()
{
    std::vector<DiskInfo> disks;

    std::ifstream file("/proc/mounts");

    if (!file.is_open())
    {
        throw std::runtime_error("Ошибка открытия /proc/mounts");
    }

    std::set<std::string> already_added_mounts;

    std::string device;
    std::string mount_point;
    std::string filesystem;
    std::string options;

    while (file >> device >> mount_point >> filesystem >> options)
    {
        mount_point = decode_mount_path(mount_point);

        if (!is_real_filesystem(filesystem))
        {
            continue;
        }

        if (already_added_mounts.count(mount_point) > 0)
        {
            continue;
        }

        try
        {
            DiskInfo disk = read_single_disk(device, mount_point, filesystem);

            if (disk.total_bytes > 0)
            {
                disks.push_back(disk);
                already_added_mounts.insert(mount_point);
            }
        }
        catch (...)
        {
            continue;
        }
    }

    std::sort(
        disks.begin(),
        disks.end(),
        [](const DiskInfo& a, const DiskInfo& b)
        {
            return a.mount_point < b.mount_point;
        }
    );

    return disks;
}

double bytes_to_gb(uint64_t bytes)
{
    return static_cast<double>(bytes) / 1024.0 / 1024.0 / 1024.0;
}