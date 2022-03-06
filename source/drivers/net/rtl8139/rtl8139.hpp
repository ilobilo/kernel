// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::net::rtl8139 {

enum regs
{
    REG_ID_0 = 0x00,
    REG_ID_1 = 0x01,
    REG_ID_2 = 0x02,
    REG_ID_3 = 0x03,
    REG_ID_4 = 0x04,
    REG_ID_5 = 0x05,
    REG_TSD_0 = 0x10,
    REG_TSD_1 = 0x14,
    REG_TSD_2 = 0x18,
    REG_TSD_3 = 0x1C,
    REG_TSAD_0 = 0x20,
    REG_TSAD_1 = 0x24,
    REG_TSAD_2 = 0x28,
    REG_TSAD_3 = 0x2C,
    REG_RBSTART = 0x30,
    REG_CR = 0x37,
    REG_CAPR = 0x38,
    REG_IMR = 0x3C,
    REG_ISR = 0x3E,
    REG_RCR = 0x44,
    REG_CONFIG1 = 0x52
};

enum cmd
{
    CMD_BUFF_EMPTY = (1 << 0),
    CMD_TX_ENABLE = (1 << 2),
    CMD_RX_ENABLE = (1 << 3),
    CMD_RESET = (1 << 4)
};

enum intmask
{
    IMR_RECEIVE_OK = (1 << 0),
    IMR_RECEIVE_ERROR = (1 << 1),
    IMR_TRANSMIT_OK = (1 << 2),
    IMR_TRANSMIT_ERROR = (1 << 3),
    IMR_RX_OVERFLOW = (1 << 4),
    IMR_LINK_CHANGE = (1 << 5),
    IMR_RX_FIFO_OVERFLOW = (1 << 6),
    IMR_CABLE_LENGTH_CHANGE = (1 << 13),
    IMR_TIME_OUT = (1 << 14),
    IMR_SYSTEM_ERROR = (1 << 15)
};

enum intstat
{
    IST_RECEIVE_OK = (1 << 0),
    IST_RECEIVE_ERR = (1 << 1),
    IST_TRANSMIT_OK = (1 << 2),
    IST_TRANSMIT_ERR = (1 << 3),
    IST_RX_BUFF_OVER = (1 << 4),
    IST_LINK_CHANGE = (1 << 5),
    IST_RX_FIFO_OVER = (1 << 6),
    IST_CABLE_LENGTH_CHANGE = (1 << 13),
    IST_TIME_OUT = (1 << 14),
    IST_SYSTEM_ERROR = (1 << 15),
};

enum rxconf
{
    RCR_PHYS_ADDR_PACKETS = (1 << 0),
    RCR_PHYS_MATCH_PACKETS = (1 << 1),
    RCR_MULTICAST_PACKETS = (1 << 2),
    RCR_BROADCAST_PACKETS = (1 << 3),
    RCR_RUNT_PACKETS = (1 << 4),
    RCR_ERROR_PACKETS = (1 << 5),
    RCR_EEPROM_SELECT = (1 << 6),
    RCR_WRAP = (1 << 7),
};

class RTL8139 : public nicmgr::NIC
{
    private:
    pci::pcidevice_t *pcidevice;
    lock_t lock;

    uint8_t BARType = 0;
    uint16_t IOBase = 0;
    uint64_t MEMBase = 0;

    uint8_t *RXBuffer = nullptr;
    uint32_t current_packet = 0;

    uint8_t TSAD[4] = { REG_TSAD_0, REG_TSAD_1, REG_TSAD_2, REG_TSAD_3 };
    uint8_t TSD[4] = { REG_TSD_0, REG_TSD_1, REG_TSD_2, REG_TSD_3 };
    size_t txcurr = 0;

    void outb(uint16_t addr, uint8_t val);
    void outw(uint16_t addr, uint16_t val);
    void outl(uint16_t addr, uint32_t val);

    uint8_t inb(uint16_t addr);
    uint16_t inw(uint16_t addr);
    uint32_t inl(uint16_t addr);

    public:
    bool initialised = false;

    void send(void *data, uint64_t length);
    void receive();

    void reset();
    void start();

    uint16_t status();
    void irq_reset();

    void read_mac();

    RTL8139(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<RTL8139*> devices;

void init();
}