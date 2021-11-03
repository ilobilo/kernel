#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/vfs/vfs.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>

using namespace kernel::drivers::display;

namespace kernel::drivers::fs::devfs {

bool initialised = false;
vfs::fs_node_t *devfs_root;

uint64_t count = 0;

vfs::fs_node_t *add(vfs::fs_t *fs, uint64_t mask, const char *name)
{
    vfs::fs_node_t *node = vfs::open_r(devfs_root, name);
    node->children.destroy();
    node->fs = fs;
    node->mask = mask;
    node->inode = count;
    count++;
    return node;
}

#pragma region null
static size_t read_null(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
{
    return 0;
}
static size_t write_null(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
{
    return 0;
}
static void open_null(vfs::fs_node_t *node, uint8_t read, uint8_t write)
{
    return;
}
static void close_null(vfs::fs_node_t *node)
{
    return;
}
static vfs::fs_t null_fs = {
    .name = "null",
    .read = &read_null,
    .write = &write_null,
    .open = &open_null,
    .close = &close_null
};
#pragma endregion null

#pragma region zero
static size_t read_zero(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
{
    memset(buffer, 0x00, size);
    return 1;
}
static size_t write_zero(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
{
    return 0;
}
static void open_zero(vfs::fs_node_t *node, uint8_t read, uint8_t write)
{
    return;
}
static void close_zero(vfs::fs_node_t *node)
{
    return;
}
static vfs::fs_t zero_fs = {
    .name = "zero",
    .read = &read_zero,
    .write = &write_zero,
    .open = &open_zero,
    .close = &close_zero
};
#pragma endregion zero

#pragma region rand
static size_t read_rand(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
{
    size_t s = 0;
    while (s < size)
    {
        buffer[s] = rand() % 0xFF;
        s++;
    }
    return size;
}
static vfs::fs_t rand_fs = {
    .name = "rand",
    .read = &read_rand
};
#pragma endregion rand

void init()
{
    serial::info("Mounting and populating DEVFS");

    if (initialised)
    {
        serial::info("DEV filesystem has already been mounted!\n");
        return;
    }

    devfs_root = vfs::mount(NULL, NULL, "/dev");

    vfs::fs_node_t *null = add(&null_fs, 0666, "null");
    null->flags = vfs::FS_CHARDEVICE;
    
    vfs::fs_node_t *zero = add(&zero_fs, 0666, "zero");
    zero->flags = vfs::FS_CHARDEVICE;

    vfs::fs_node_t *rand = add(&rand_fs, 0444, "random");
    rand->flags = vfs::FS_CHARDEVICE;
    rand->length = 1024;

    vfs::fs_node_t *urand = add(&rand_fs, 0444, "urandom");
    urand->flags = vfs::FS_CHARDEVICE;
    urand->length = 1024;

    serial::newline();
    initialised = true;
}
}