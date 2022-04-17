// Copyright (C) 2021-2022  ilobilo

#include <lib/terminal.hpp>

int64_t terminal_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    lockit(this->lock);

    while (offset--)
    {
        while (this->bigbuff.empty());
        this->bigbuff.get();
    }

    bool wait = true;
    for (size_t i = 0; i < size; i++)
    {
        if (this->bigbuff.empty())
        {
            if (wait == true)
            {
                this->lock.unlock();
                while (this->bigbuff.empty());
                this->lock.lock();
            }
            else return i;
        }
        *buffer = this->bigbuff.get();
        buffer++;
        wait = false;
    }

    return size;
}

int64_t terminal_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    this->print("%.*s", static_cast<int>(size), buffer);
    return size;
}

int terminal_res::ioctl(void *handle, uint64_t request, void *argp)
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

bool terminal_res::grow(void *handle, size_t new_size)
{
    return false;
}

void terminal_res::unref(void *handle)
{
    this->refcount--;
}

void terminal_res::link(void *handle)
{
    this->stat.nlink++;
}

void terminal_res::unlink(void *handle)
{
    this->stat.nlink--;
}

void *terminal_res::mmap(uint64_t page, int flags)
{
    return nullptr;
}

void terminal_res::add_char(char c)
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
                if (this->tios.c_lflag & ECHO) this->print("%c", c);
                while (!this->buff.empty() && !this->bigbuff.full())
                {
                    char ch = this->buff.get();
                    this->bigbuff.put(ch);
                }
                this->buff.clear();
                return;
            case '\b':
                if (this->buff.empty()) return;
                char oldchar = this->buff.get_back();
                if (this->tios.c_lflag & ECHO)
                {
                    this->print("\b \b");
                    if (oldchar >= 0x01 && oldchar <= 0x1A) this->print("\b \b");
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
        if (c >= 0x20 && c <= 0x7E) this->print("%c", c);
        else if (c >= 0x01 && c <= 0x1A) this->print("^%c", c + 0x40);
    }
}

void terminal_res::add_str(const char *str)
{
    while (*str)
    {
        this->add_char(*str);
        str++;
    }
}

string terminal_res::getline()
{
    std::string ret("");
    char c = 0;

    while (true)
    {
        this->read(nullptr, reinterpret_cast<uint8_t*>(&c), 0, 1);
        if (c == '\n') break;
        ret.append(c);
    }

    return ret;
}

terminal_res::terminal_res(uint16_t rows, uint64_t columns)
{
    this->stat.size = 0;
    this->stat.blocks = 0;
    this->stat.blksize = 512;
    this->stat.rdev = vfs::dev_new_id();
    this->stat.mode = 0666 | vfs::ifchr;
    this->can_mmap = true;

    this->tios.c_lflag = ISIG | ICANON | ECHO;
    this->tios.c_cc[VEOF] = 0x04;
    this->tios.c_cc[VEOL] = 0x00;
    this->tios.c_cc[VERASE] = 0x7F;
    this->tios.c_cc[VINTR] = 0x03;
    this->tios.c_cc[VKILL] = 0x15;
    this->tios.c_cc[VMIN] = 0x01;
    this->tios.c_cc[VQUIT] = 0x1C;
    this->tios.c_cc[VSTART] = 0x11;
    this->tios.c_cc[VSTOP] = 0x13;
    this->tios.c_cc[VSUSP] = 0x1A;
    this->tios.c_cc[VTIME] = 0x00;
    this->tios.c_cc[VLNEXT] = 0x16;
    this->tios.c_cc[VWERASE] = 0x17;

    this->wsize.ws_row = rows;
    this->wsize.ws_col = columns;
}