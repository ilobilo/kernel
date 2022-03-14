// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/vfs/vfs.hpp>
#include <lib/string.hpp>
#include <lib/cwalk.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::fs::vfs {

bool initialised = false;
bool debug = false;
fs_node_t *fs_root;
new_lock(vfs_lock)

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
    if (node->fs->open != 0) return node->fs->open(node, read, write);
    else return;
}

void close_fs(fs_node_t *node)
{
    if (node->fs->close != 0) return node->fs->close(node);
    else return;
}

fs_node_t *getchild(fs_node_t *parent, const char *path)
{
    fs_node_t *parent_node;
    fs_node_t *child_node;
    if (!parent) parent_node = fs_root;
    else parent_node = parent;

    if (*path == '\0') return nullptr;
    for (size_t i = 0; i < parent_node->children.size(); i++)
    {
        child_node = parent_node->children[i];
        if (!strcmp(child_node->name, path)) return child_node;
    }
    return nullptr;
}

fs_node_t *add_new_child(fs_node_t *parent, const char *name)
{
    if (!parent) parent = fs_root;
    fs_node_t *node = static_cast<fs_node_t*>(calloc(1, sizeof(fs_node_t)));
    strcpy(node->name, name);
    node->parent = parent;
    node->fs = parent->fs;
    parent->children.push_back(node);
    return node;
}

void remove_child(fs_node_t *parent, const char *name)
{
    if (!parent) parent = fs_root;
    for (size_t i = 0; i < parent->children.size(); i++)
    {
        fs_node_t *node = parent->children[i];
        if (!strcmp(node->name, name))
        {
            for (size_t i = 0; i < node->children.size(); i++) free(node->children[i]);
            node->children.destroy();
            free(node);
            parent->children.remove(i);
            return;
        }
    }
}

char* node2path(fs_node_t *node)
{
    vector<char*> pathvec;
    fs_node_t *parent = node;
    if (parent == fs_root->ptr || parent == fs_root || parent == nullptr)
    {
        char *path = new char[2] { "/" };
        return path;
    }
    while (parent != fs_root->ptr && parent != fs_root && parent != nullptr)
    {
        pathvec.push_back(parent->name);
        pathvec.push_back("/"_c);
        parent = parent->parent;
    }
    pathvec.reverse();
    size_t size = 1;
    for (char *name : pathvec) size += strlen(name);
    char *path = new char[size];
    for (char *name : pathvec) strcat(path, name);
    pathvec.destroy();
    return path;
}

[[gnu::constructor]] void set_cwalk_style()
{
    cwk_path_set_style(CWK_STYLE_UNIX);
}

fs_node_t *open(fs_node_t *parent, const char *path, bool create)
{
    if ((parent == nullptr || parent == fs_root) && !strcmp(path, "/") && fs_root) return fs_root->ptr;
    lockit(vfs_lock);

    if (path == nullptr)
    {
        if (debug) error("VFS: Invalid path!");
        return nullptr;
    }

    fs_node_t *parent_node;
    fs_node_t *child_node;

    if (parent == nullptr && fs_root) parent_node = fs_root->ptr;
    else parent_node = parent;

    if (parent_node == nullptr)
    {
        if (debug)
        {
            error("VFS: Couldn't find directory /");
            error("VFS: Is root mounted?");
        }
        return nullptr;
    }

    size_t length = strlen(path) + 1;
    char *_path = nullptr;

    if (cwk_path_is_relative(path))
    {
        length = cwk_path_get_absolute(node2path(parent), path, nullptr, 0) + 1;
        _path = new char[length];
        cwk_path_get_absolute(node2path(parent), path, _path, length);
    }
    else
    {
        _path = new char[length];
        strcpy(_path, path);
    }

    cwk_path_normalize(_path, const_cast<char*>(_path), length);
    parent_node = fs_root->ptr;

    if (!strcmp(_path, "/"))
    {
        delete[] _path;
        return fs_root->ptr;
    }

    vector<const char*> segments;
    cwk_segment segment;

    if(!cwk_path_get_first_segment(_path, &segment))
    {
        if (debug) error("VFS: Path doesn't have any segments!");
        delete[] _path;
        return nullptr;
    }

    do {
        char *seg = new char[segment.size];
        strncpy(seg, segment.begin, segment.size);
        segments.push_back(seg);
    } while(cwk_path_get_next_segment(&segment));
    delete[] _path;

    size_t i = 0;
    next:
    if (i < segments.size())
    {
        for (size_t t = 0; t < parent_node->children.size(); t++)
        {
            child_node = parent_node->children[t];
            if (!strcmp(segments[i], child_node->name))
            {
                while ((child_node->flags & 0x07) == FS_MOUNTPOINT || (child_node->flags & 0x07) == FS_SYMLINK) child_node = child_node->ptr;
                parent_node = child_node;
                i++;
                goto next;
            }
        }
        if (create) add_new_child(parent_node, segments[i]);
        else goto notfound;
        goto next;
    }

    for (const char *&seg : segments) delete seg;
    segments.destroy();

    return child_node;

    notfound:
    if (debug) error("VFS: File not found!");
    for (const char *&seg : segments) delete seg;
    segments.destroy();

    return nullptr;
}

fs_node_t *mount_root(fs_t *fs)
{
    fs_node_t *node = add_new_child(fs_root, "/");
    fs_root->ptr = node;
    node->flags = FS_DIRECTORY;
    node->fs = fs;
    return node;
}

fs_node_t *mount(fs_t *fs, fs_node_t *parent, const char *path)
{
    if (!fs_root->ptr) mount_root(nullptr);
    if (!parent) parent = fs_root->ptr;
    if (!fs) fs = parent->fs;
    parent->ptr = open(parent, path, true);
    parent->flags = FS_MOUNTPOINT;
    parent->ptr->flags = FS_DIRECTORY;
    parent->ptr->fs = fs;
    return parent->ptr;
}

void init()
{
    log("Installing VFS");

    if (initialised)
    {
        warn("VFS has already been installed!\n");
        return;
    }

    fs_root = new fs_node_t;
    fs_root->flags = filetypes::FS_MOUNTPOINT;
    strcpy(fs_root->name, ROOTNAME);
    fs_root->fs = nullptr;

    serial::newline();
    initialised = true;
}
}