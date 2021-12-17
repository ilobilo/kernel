// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/vfs/vfs.hpp>
#include <lib/string.hpp>
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
    if (mask) node->mask = mask;
    node->inode = count;
    count++;
    return node;
}

#pragma region null
static size_t read_null([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, [[gnu::unused]] size_t size, [[gnu::unused]] char *buffer)
{
    return 0;
}
static size_t write_null([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, [[gnu::unused]] size_t size, [[gnu::unused]] char *buffer)
{
    return 0;
}
static void open_null([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] uint8_t read, [[gnu::unused]] uint8_t write)
{
    return;
}
static void close_null([[gnu::unused]] vfs::fs_node_t *node)
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
static size_t read_zero([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, [[gnu::unused]] size_t size, char *buffer)
{
    memset(buffer, 0x00, size);
    return 1;
}
static size_t write_zero([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, [[gnu::unused]] size_t size, [[gnu::unused]] char *buffer)
{
    return 0;
}
static void open_zero([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] uint8_t read, [[gnu::unused]] uint8_t write)
{
    return;
}
static void close_zero([[gnu::unused]] vfs::fs_node_t *node)
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
static size_t read_rand([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, size_t size, char *buffer)
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

#pragma region tty
static size_t read_ttys([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, [[gnu::unused]] size_t size, [[gnu::unused]] char *buffer)
{
    return 0;
}
static size_t write_ttys([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, size_t size, char *buffer)
{
    if (!size) size = strlen(buffer);
    serial::serial_printf("%.*s", static_cast<int>(size), buffer);
    return size;
}
static vfs::fs_t ttys_fs = {
    .name = "ttys",
    .read = &read_ttys,
    .write = &write_ttys
};

static size_t read_tty([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, [[gnu::unused]] size_t size, [[gnu::unused]] char *buffer)
{
    return 0;
}
static size_t write_tty([[gnu::unused]] vfs::fs_node_t *node, [[gnu::unused]] size_t offset, size_t size, char *buffer)
{
    if (!size) size = strlen(buffer);
    printf("%.*s", static_cast<int>(size), buffer);
    return size;
}
static vfs::fs_t tty_fs = {
    .name = "tty",
    .read = &read_tty,
    .write = &write_tty
};
static void addtty(const char *name, bool serial)
{
    vfs::fs_node_t *tty;
    if (!serial) tty = add(&tty_fs, 0, name);
    else tty = add(&ttys_fs, 0, name);
    tty->flags = vfs::FS_CHARDEVICE;
}
#pragma endregion tty

void init()
{
    serial::info("Mounting and populating DEVFS");

    if (initialised)
    {
        serial::warn("DEV filesystem has already been mounted!\n");
        return;
    }

    devfs_root = vfs::mount(0, 0, "/dev");
    devfs_root->mask = 0755;

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

    addtty("ttyS0", true);
    addtty("tty0", false);
    addtty("console", false);

    serial::newline();
    initialised = true;
}
}