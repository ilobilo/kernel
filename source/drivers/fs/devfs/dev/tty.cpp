// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/fs/devfs/dev/tty.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <kernel/kernel.hpp>

namespace kernel::drivers::fs::dev::tty {

bool initialised = false;
tty_res *current_tty = nullptr;
static size_t next_id = 0;

int64_t tty_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    while (offset--)
    {
        while (this->bigbuff.empty());
        this->bigbuff.get();
    }
    for (size_t i = 0; i < size; i++)
    {
        while (this->bigbuff.empty());
        *buffer = this->bigbuff.get();
        buffer++;
    }

    return size;
}

int64_t tty_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    printf(this->thisterm, "%.*s", static_cast<int>(size), buffer);
    return size;
}

int tty_res::ioctl(void *handle, uint64_t request, void *argp)
{
    lockit(this->lock);

    switch (request)
    {
        case TIOCGWINSZ:
            memcpy(argp, &this->wsize, sizeof(winsize));
            break;
        case TIOCSWINSZ:
            memcpy(&this->wsize, argp, sizeof(winsize));
            // TODO: SIGWINCH
            break;
        case TCGETS:
            memcpy(argp, &this->tios, sizeof(termios));
            break;
        case TCSETS:
        case TCSETSW:
        case TCSETSF:
            memcpy(&this->tios, argp, sizeof(termios));
            break;
        default:
            return vfs::default_ioctl(handle, request, argp);
    }
    return 0;
}

bool tty_res::grow(void *handle, size_t new_size)
{
    return false;
}

void tty_res::unref(void *handle)
{
    this->refcount--;
}

void tty_res::link(void *handle)
{
    this->stat.nlink++;
}

void tty_res::unlink(void *handle)
{
    this->stat.nlink--;
}

void *tty_res::mmap(uint64_t page, int flags)
{
    return nullptr;
}

void tty_res::add_char(char c)
{
    lockit(this->lock);

    if (this->tios.c_iflag & ISTRIP) c &= 0x7F;
    if ((this->tios.c_iflag & IGNCR) && c == '\r') return;

    if ((this->tios.c_iflag & INLCR) && c == '\n') c = '\r';
    else if ((this->tios.c_iflag & INLCR) && c == '\r') c = '\n';

    if (this->tios.c_lflag & ICANON)
    {
        switch (c)
        {
            case '\n':
                if (this->buff.full()) return;
                this->buff.put(c);
                if (this->tios.c_lflag & ECHO) printf(this->thisterm, "%c", c);
                while (!this->buff.empty()) this->bigbuff.put(this->buff.get());
                return;
            case '\b':
                if (this->buff.empty()) return;
                char oldchar = this->buff.get();
                if (this->tios.c_lflag & ECHO)
                {
                    printf(this->thisterm, "\b \b");
                    if (oldchar >= 0x01 && oldchar <= 0x1A) printf(this->thisterm, "\b \b");
                }
                return;
        }

        if (this->buff.full()) return;
        this->buff.put(c);
    }
    else
    {
        if (this->bigbuff.full()) return;
        this->bigbuff.put(c);
    }

    if (this->tios.c_lflag & ECHO)
    {
        if (c >= 0x20 && c <= 0x7E) printf(this->thisterm, "%c", c);
        else if (c >= 0x01 && c <= 0x1A) printf(this->thisterm, "^%c", c + 0x40);
    }
}

void tty_res::add_str(const char *str)
{
    while (*str)
    {
        this->add_char(*str);
        str++;
    }
}

string tty_res::getline()
{
    std::string ret("");
    char c = 0;

    while (true)
    {
        current_tty->read(nullptr, reinterpret_cast<uint8_t*>(&c), 0, 1);
        if (c == '\n') break;
        else ret.append(c);
    }

    return ret;
}

void init()
{
    if (initialised) return;

    for (size_t i = 0; i < terminal::term_count; i++)
    {
        tty_res *tty = new tty_res;

        tty->id = next_id++;
        tty->thisterm = terminal::terminals[i];

        tty->stat.size = 0;
        tty->stat.blocks = 0;
        tty->stat.blksize = 512;
        tty->stat.rdev = vfs::dev_new_id();
        tty->stat.mode = 0666 | vfs::ifchr;
        tty->can_mmap = true;

        tty->tios.c_lflag = ISIG | ICANON | ECHO;
        tty->tios.c_cc[VEOF] = 0x04;
        tty->tios.c_cc[VEOL] = 0x00;
        tty->tios.c_cc[VERASE] = 0x7F;
        tty->tios.c_cc[VINTR] = 0x03;
        tty->tios.c_cc[VKILL] = 0x15;
        tty->tios.c_cc[VMIN] = 0x01;
        tty->tios.c_cc[VQUIT] = 0x1C;
        tty->tios.c_cc[VSTART] = 0x11;
        tty->tios.c_cc[VSTOP] = 0x13;
        tty->tios.c_cc[VSUSP] = 0x1A;
        tty->tios.c_cc[VTIME] = 0x00;
        tty->tios.c_cc[VLNEXT] = 0x16;
        tty->tios.c_cc[VWERASE] = 0x17;

        tty->wsize.ws_row = terminal::terminals[i]->rows;
        tty->wsize.ws_col = terminal::terminals[i]->columns;

        std::string ttyname("tty");
        ttyname.push_back(i + 1 + '0');
        devfs::add(tty, ttyname);
        if (current_tty == nullptr) current_tty = tty;
    }

    initialised = true;
}
}