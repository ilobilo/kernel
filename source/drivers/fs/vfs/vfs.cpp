#include <drivers/fs/vfs/vfs.hpp>

namespace kernel::drivers::fs::vfs {

fs_node_t *fs_root = 0;

uint64_t read_fs(fs_node_t *node, uint64_t offset, uint64_t size, char *buffer)
{
    if (node->fs->read != 0) return node->fs->read(node, offset, size, buffer);
    else return 0;
}

uint64_t write_fs(fs_node_t *node, uint64_t offset, uint64_t size, char *buffer)
{
    if (node->fs->write != 0) return node->fs->write(node, offset, size, buffer);
    else return 0;
}

void open_fs(fs_node_t *node, uint8_t read, uint8_t write)
{
    if (node->fs->open != 0) return node->fs->open(node);
    else return;
}

void close_fs(fs_node_t *node)
{
    if (node->fs->close != 0) return node->fs->close(node);
    else return;
}

dirent_t *readdir_fs(fs_node_t *node, uint64_t index)
{
    if ((node->flags & 0x7) == FS_DIRECTORY && node->fs->readdir != 0) return node->fs->readdir(node, index);
    else return 0;
}

fs_node_t *finddir_fs(fs_node_t *node, char *name)
{
    if ((node->flags & 0x7) == FS_DIRECTORY && node->fs->finddir != 0) return node->fs->finddir(node, name);
    else return 0;
}
}