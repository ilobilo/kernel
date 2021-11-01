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

fs_node_t *getchild(fs_node_t *parent, const char *path)
{
    fs_node_t *parent_node;
    fs_node_t *child_node;
    if (!parent) parent_node = fs_root;
    else parent_node = parent;

    if (*path == '\0')
    {
        release_lock(&vfs_lock);
        return NULL;
    }
    for (size_t i = 0; i < parent_node->children.size(); i++)
    {
        child_node = parent_node->children.at(i);
        if (!strcmp(child_node->name, path)) return child_node;
    }
    return NULL;
}

fs_node_t *add_new_child(fs_node_t *parent, const char *name)
{
    if (!parent) parent = fs_root;
    fs_node_t *node = (fs_node_t*)heap::calloc(1, sizeof(fs_node_t));
    strcpy(node->name, name);
    node->parent = parent;
    node->fs = parent->fs;
    node->children.init(1);
    parent->children.push_back(node);
    return node;
}

void remove_child(fs_node_t *parent, const char *name)
{
    if (!parent) parent = fs_root;
    for (size_t i = 0; i < parent->children.size(); i++)
    {
        fs_node_t *node = parent->children.at(i);
        if (!strcmp(node->name, name))
        {
            node->children.destroy();
            heap::free(node);
            parent->children.remove(i);
            return;
        }
    }
}

fs_node_t *open(fs_node_t *parent, const char *path)
{
    acquire_lock(&vfs_lock);
    if (!strcmp(path, "/")) return fs_root->ptr;
    if (!strcmp(path, "[ROOT]")) return fs_root;
    if (!path || path[0] != '/')
    {
        serial::err("VFS: Paths must start with /");
        release_lock(&vfs_lock);
        return NULL;
    }
    if (strchr(path, ' '))
    {
        serial::err("VFS: Paths must not contain spaces!");
        release_lock(&vfs_lock);
        return NULL;
    }

    fs_node_t *parent_node;
    fs_node_t *child_node;
    size_t slashes;
    size_t cleared = 0;

    if (!parent) parent_node = fs_root->ptr;
    else parent_node = parent;
    if (!parent_node)
    {
        serial::err("VFS: Couldn't find directory /");
        serial::err("VFS: Is root mounted?");
        release_lock(&vfs_lock);
        return NULL;
    }

    char **patharr = strsplit_count(path, "/", &slashes);
    while (!strcmp(patharr[slashes], "") || !patharr[slashes]) slashes--;
    patharr++;

    while (slashes)
    {
        next:
        if ((parent_node->flags & 0x07) == FS_MOUNTPOINT || (parent_node->flags & 0x07) == FS_SYMLINK)
        {
            parent_node = parent_node->ptr;
            slashes++;
            goto next;
        }
        for (size_t i = 0; i < parent_node->children.size(); i++)
        {
            child_node = parent_node->children.at(i);
            if (!strcmp(child_node->name, patharr[cleared]))
            {
                parent_node = parent_node->children.at(i);
                cleared++;
                goto next;
            }
            if (i + 1 == parent_node->children.size()) goto notfound;
        }
        slashes--;
    }

    release_lock(&vfs_lock);
    return child_node;

    notfound:
    serial::err("VFS: File not found!");
    release_lock(&vfs_lock);
    return NULL;
}

fs_node_t *create(fs_node_t *parent, const char *path)
{
    acquire_lock(&vfs_lock);
    if (!path || path[0] != '/')
    {
        serial::err("VFS: Paths must start with /");
        release_lock(&vfs_lock);
        return NULL;
    }
    if (strchr(path, ' '))
    {
        serial::err("VFS: Paths must not contain spaces!");
        release_lock(&vfs_lock);
        return NULL;
    }

    fs_node_t *parent_node;
    fs_node_t *child_node;
    size_t slashes;
    size_t cleared = 0;

    if (!parent) parent_node = fs_root->ptr;
    else parent_node = parent;
    if (!parent_node)
    {
        serial::err("VFS: Couldn't find directory /");
        serial::err("VFS: Is root mounted?");
        release_lock(&vfs_lock);
        return NULL;
    }

    char **patharr = strsplit_count(path, "/", &slashes);
    while (!strcmp(patharr[slashes], "") || !patharr[slashes]) slashes--;
    patharr++;

    while (slashes)
    {
        if (getchild(parent_node, patharr[cleared]))
        {
            cleared++;
            slashes--;
            continue;
        }
        if (slashes > 1) parent_node = add_new_child(parent_node, patharr[cleared]);
        else child_node = add_new_child(parent_node, patharr[cleared]);

        cleared++;
        slashes--;
    }

    release_lock(&vfs_lock);
    return child_node;
}

fs_node_t *mount_root(fs_t *fs)
{
    fs_node_t *node = add_new_child(fs_root, "/");
    fs_root->ptr = node;
    node->flags = FS_DIRECTORY;
    node->fs = fs;
    return node;
}

fs_node_t *mount(fs_t *fs, fs_node_t *parent, fs_node_t *node)
{
    if (!parent) parent = fs_root;
    parent->ptr = node;
    parent->flags = FS_MOUNTPOINT;
    parent->ptr->flags = FS_DIRECTORY;
    return node;
}

void init()
{
    serial::info("Initialising Virtual filesystem");

    fs_root = (fs_node_t*)heap::malloc(sizeof(fs_node_t));
    fs_root->flags = filetypes::FS_MOUNTPOINT;
    fs_root->children.init(1);
    strcpy(fs_root->name, "[ROOT]");
    fs_root->fs = NULL;

    serial::newline();
    initialised = true;
}
}