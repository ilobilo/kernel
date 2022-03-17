// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::net::rtl8169 {

enum regs
{
    REG_ID_0 = 0x00,
    REG_ID_1 = 0x01,
    REG_ID_2 = 0x02,
    REG_ID_3 = 0x03,
    REG_ID_4 = 0x04,
    REG_ID_5 = 0x05,
    REG_TNPDS_1 = 0x20,
    REG_TNPDS_2 = 0x24,
    REG_CR = 0x37,
    REG_TPPOLL = 0x38,
    REG_IMR = 0x3C,
    REG_ISR = 0x3E,
    REG_TCR = 0x40,
    REG_RCR = 0x44,
    REG_9346CR = 0x50,
    REG_CONFIG1 = 0x52,
    REG_RMS = 0xDA,
    REG_RDSAR_1 = 0xE4,
    REG_RDSAR_2 = 0xE8,
    REG_ETTHR = 0xEC
};

enum cmd
{
    CMD_TX_ENABLE = (1 << 2),
    CMD_RX_ENABLE = (1 << 3),
    CMD_RESET = (1 << 4)
};

enum intstat
{
    IST_RECEIVE_OK = (1 << 0),
    IST_RECEIVE_ERR = (1 << 1),
    IST_TRANSMIT_OK = (1 << 2),
    IST_TRANSMIT_ERR = (1 << 3),
    IST_RX_UNAVAIL = (1 << 4),
    IST_LINK_CHANGE = (1 << 5),
    IST_RX_FIFO_OVER = (1 << 6),
    IST_TX_UNAVAIL = (1 << 7),
    IST_SOFT_INT = (1 << 8),
    IST_TIME_OUT = (1 << 14),
    IST_SYSTEM_ERROR = (1 << 15),
};

enum desc
{
    RTL8169_OWN = (1 << 31),
    RTL8169_EOR = (1 << 30),
    RTL8169_FFR = (1 << 29),
    RTL8169_LFR = (1 << 28)
};

static constexpr uint8_t RTL8169_NUM_RX_DESC = 32;
static constexpr uint8_t RTL8169_NUM_TX_DESC = 8;

static constexpr uint32_t RTL8169_RX_BUFF_SIZE = 8192;

struct [[gnu::packed]] Desc
{
    uint32_t command;
    uint32_t vlan;
    uint64_t buffer;
};

class RTL8169 : public nicmgr::NIC
{
    private:
    pci::pcidevice_t *pcidevice;
    lock_t lock;

    uint8_t BARType = 0;
    uint16_t IOBase = 0;
    uint64_t MEMBase = 0;

    Desc *rxdescs[RTL8169_NUM_RX_DESC];
    Desc *txdescs[RTL8169_NUM_TX_DESC];

    uint16_t rxcurr = 0;
    uint16_t txcurr = 0;

    void outb(uint16_t addr, uint8_t val);
    void outw(uint16_t addr, uint16_t val);
    void outl(uint16_t addr, uint32_t val);

    uint8_t inb(uint16_t addr);
    uint16_t inw(uint16_t addr);
    uint32_t inl(uint16_t addr);

    void rxinit();
    void txinit();

    public:
    bool initialised = false;

    void send(void *data, uint64_t length);
    void receive();

    void reset();
    void start();

    uint16_t status();
    void irq_reset(uint16_t status);

    void read_mac();

    RTL8169(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<RTL8169*> devices;

void init();
}