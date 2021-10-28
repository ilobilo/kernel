#include <drivers/display/serial/serial.hpp>
#include <system/sched/lock/lock.hpp>
#include <drivers/fs/vfs/vfs.hpp>
#include <lib/string.hpp>

using namespace kernel::drivers::display;

namespace kernel::drivers::fs::vfs {

bool initialised = false;
fs_node_t *fs_root;
DEFINE_LOCK(vfs_lock);

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

static size_t strlen_slash(const char *string)
{
    bool skip1 = false;
    if (!strncmp(string, "/", 1)) skip1 = true;
    size_t counter = 0;
    again:
    while (*string != '\0' && *string != '/')
    {
        string++;
        counter++;
    };
    if (skip1)
    {
        skip1 = false;
        string++;
        counter++;
        goto again;
    }
    return counter;
}

fs_node_t *path2node(fs_node_t *parent, const char *path)
{
    acquire_lock(&vfs_lock);
    fs_node_t *parent_node;
    fs_node_t *child_node;
    if (!parent) parent_node = fs_root;
    else parent_node = parent;

    while (true)
    {
        next:
        if (*path == '\0')
        {
            release_lock(&vfs_lock);
            return NULL;
        }
        for (size_t i = 0; i < parent_node->children.size(); i++)
        {
            child_node = parent_node->children.at(i);
            size_t length = strlen_slash(path);
            if (!strncmp(child_node->name, path, length))
            {
                path += length;
                if (*(path++) == '\0')
                {
                    release_lock(&vfs_lock);
                    return child_node;
                }
                parent_node = child_node;
                goto next;
            }
        }
        release_lock(&vfs_lock);
        return NULL;
    }
    return NULL;
}

void init()
{
    serial::info("Initialising virtual filesystem");

    fs_root = (fs_node_t*)heap::malloc(sizeof(fs_node_t));
    fs_root->flags = filetypes::FS_DIRECTORY;
    fs_root->children.init();
    strcpy(fs_root->name, "[root]");
    fs_root->fs = NULL;

    serial::newline();
    initialised = true;
}
}