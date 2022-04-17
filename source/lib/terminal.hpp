// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/vfs/vfs.hpp>
#include <lib/ring.hpp>

using namespace kernel::system;

static constexpr uint8_t nccs = 32;

enum requests
{
    TCGETS = 0x5401,
    TCSETS = 0x5402,
    TCSETSW = 0x5403,
    TCSETSF = 0x5404,
    TIOCGWINSZ = 0x5413,
    TIOCSWINSZ = 0x5414,
};

enum input_modes
{
    BRKINT = 0000001,
    ICRNL = 0000002,
    IGNBRK = 0000004,
    IGNCR = 0000010,
    IGNPAR = 0000020,
    INLCR = 0000040,
    INPCK = 0000100,
    ISTRIP = 0000200,
    IUCLC = 0000400,
    IXANY = 0001000,
    IXOFF = 0002000,
    IXON = 0004000,
    PARMRK = 0010000
};

enum output_modes
{
    OPOST = 0000001,
    OLCUC = 0000002,
    ONLCR = 0000004,
    OCRNL = 0000010,
    ONOCR = 0000020,
    ONLRET = 0000040,
    OFILL = 0000100,
    OFDEL = 0000200,
    NLDLY = 0000400,
    NL0 = 0000000,
    NL1 = 0000400,
    CRDLY = 0003000,
    CR0 = 0000000,
    CR1 = 0001000,
    CR2 = 0002000,
    CR3 = 0003000,
    TABDLY = 0014000,
    TAB0 = 0000000,
    TAB1 = 0004000,
    TAB2 = 0010000,
    TAB3 = 0014000,
    BSDLY = 0020000,
    BS0 = 0000000,
    BS1 = 0020000,
    FFDLY = 0100000,
    FF0 = 0000000,
    FF1 = 0100000,
    VTDLY = 0040000,
    VT0 = 0000000,
    VT1 = 0040000
};

enum baud_rates
{
    B0 = 0000000,
    B50 = 0000001,
    B75 = 0000002,
    B110 = 0000003,
    B134 = 0000004,
    B150 = 0000005,
    B200 = 0000006,
    B300 = 0000007,
    B600 = 0000010,
    B1200 = 0000011,
    B1800 = 0000012,
    B2400 = 0000013,
    B4800 = 0000014,
    B9600 = 0000015,
    B19200 = 0000016,
    B38400 = 0000017
};

enum control_modes
{
    CSIZE = 0000060,
    CS5 = 0000000,
    CS6 = 0000020,
    CS7 = 0000040,
    CS8 = 0000060,
    CSTOPB = 0000100,
    CREAD = 0000200,
    PARENB = 0000400,
    PARODD = 0001000,
    HUPCL = 0002000,
    CLOCAL = 0004000
};

enum local_modes
{
    ISIG = 0000001,
    ICANON = 0000002,
    XCASE = 0000004,
    ECHO = 0000010,
    ECHOE = 0000020,
    ECHOK = 0000040,
    ECHONL = 0000100,
    NOFLSH = 0000200,
    TOSTOP = 0000400,
    IEXTEN = 0001000
};

enum ccs
{
    VEOF = 1,
    VEOL = 2,
    VERASE = 3,
    VINTR = 4,
    VKILL = 5,
    VMIN = 6,
    VQUIT = 7,
    VSTART = 8,
    VSTOP = 9,
    VSUSP = 10,
    VTIME = 11,
    VLNEXT = 12,
    VWERASE = 13
};

struct winsize
{
    uint16_t ws_row;
    uint16_t ws_col;
    uint16_t ws_xpixel;
    uint16_t ws_ypixel;
};

struct termios
{
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_cflag;
    uint32_t c_lflag;
    uint8_t c_cc[nccs];
};

struct terminal_res : vfs::resource_t
{
    ringbuffer<char> buff;
    ringbuffer<char> bigbuff;
    winsize wsize;
    termios tios;
    lock_t read_lock;

    virtual int print(const char *fmt, ...)
    {
        return 0;
    }

    int64_t read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size);
    int64_t write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size);
    int ioctl(void *handle, uint64_t request, void *argp);
    bool grow(void *handle, size_t new_size);
    void unref(void *handle);
    void link(void *handle);
    void unlink(void *handle);
    void *mmap(uint64_t page, int flags);

    void add_char(char c);
    void add_str(const char *str);
    std::string getline();

    terminal_res(uint16_t rows, uint64_t columns);
};