// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/net/nicmgr/nicmgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::net::e1000 {

#define TSTA_DD (1 << 0)
#define TSTA_EC (1 << 1)
#define TSTA_LC (1 << 2)
#define LSTA_TU (1 << 3)
#define ECTRL_SLU 0x40

enum regs
{
    REG_CTRL = 0x0000,
    REG_STATUS = 0x0008,
    REG_EEPROM = 0x0014,
    REG_CTRL_EXT = 0x0018,
    REG_IMASK = 0x00D0,
    REG_RCTRL = 0x0100,
    REG_RXDESCLO = 0x2800,
    REG_RXDESCHI = 0x2804,
    REG_RXDESCLEN = 0x2808,
    REG_RXDESCHEAD = 0x2810,
    REG_RXDESCTAIL = 0x2818,
    REG_TCTRL = 0x0400,
    REG_TXDESCLO = 0x3800,
    REG_TXDESCHI = 0x3804,
    REG_TXDESCLEN = 0x3808,
    REG_TXDESCHEAD = 0x3810,
    REG_TXDESCTAIL = 0x3818,
    REG_RDTR = 0x2820,
    REG_RXDCTL = 0x3828,
    REG_RADV = 0x282C,
    REG_RSRPD = 0x2C00,
    REG_TIPG = 0x0410,
};

enum rctl
{
    RCTL_EN = (1 << 1),
    RCTL_SBP = (1 << 2),
    RCTL_UPE = (1 << 3),
    RCTL_MPE = (1 << 4),
    RCTL_LPE = (1 << 5),
    RCTL_LBM_NONE = (0 << 6),
    RCTL_LBM_PHY = (3 << 6),
    RCTL_RDMTS_HALF = (0 << 8),
    RCTL_RDMTS_QUARTER = (1 << 8),
    RCTL_RDMTS_EIGHTH = (2 << 8),
    RCTL_MO_36 = (0 << 12),
    RCTL_MO_35 = (1 << 12),
    RCTL_MO_34 = (2 << 12),
    RCTL_MO_32 = (3 << 12),
    RCTL_BAM = (1 << 15),
    RCTL_VFE = (1 << 18),
    RCTL_CFIEN = (1 << 19),
    RCTL_CFI = (1 << 20),
    RCTL_DPF = (1 << 22),
    RCTL_PMCF = (1 << 23),
    RCTL_SECRC = (1 << 26),
    RCTL_BSIZE_256 = (3 << 16),
    RCTL_BSIZE_512 = (2 << 16),
    RCTL_BSIZE_1024 = (1 << 16),
    RCTL_BSIZE_2048 = (0 << 16),
    RCTL_BSIZE_4096 = ((3 << 16) | (1 << 25)),
    RCTL_BSIZE_8192 = ((2 << 16) | (1 << 25)),
    RCTL_BSIZE_16384 = ((1 << 16) | (1 << 25))
};

enum tctl
{
    TCTL_EN = (1 << 1),
    TCTL_PSP = (1 << 3),
    TCTL_CT_SHIFT = 4,
    TCTL_COLD_SHIFT = 12,
    TCTL_SWXOFF = (1 << 22),
    TCTL_RTLC = (1 << 24)
};

enum cmd
{
    CMD_EOP = (1 << 0),
    CMD_IFCS = (1 << 1),
    CMD_IC = (1 << 2),
    CMD_RS = (1 << 3),
    CMD_RPS = (1 << 4),
    CMD_VLE = (1 << 6),
    CMD_IDE = (1 << 7)
};

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8
struct [[gnu::packed]] RXDesc
{
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
};

struct [[gnu::packed]] TXDesc
{
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
};

class E1000 : public nicmgr::NetCard
{
    private:
    pci::pcidevice_t *pcidevice;
    volatile lock_t lock;

    uint8_t BARType = 0;
    uint16_t IOBase = 0;
    uint64_t MEMBase = 0;

    RXDesc *rxdescs[E1000_NUM_RX_DESC];
    TXDesc *txdescs[E1000_NUM_TX_DESC];
    uint16_t rxcurr = 0;
    uint16_t txcurr = 0;

    void rxinit();
    void txinit();

    bool eeprom = false;
    bool detecteeprom();
    uint32_t readeeprom(uint8_t addr);
    bool read_mac();

    void outcmd(uint16_t addr, uint32_t val);
    uint32_t incmd(uint16_t addr);

    void intenable();

    public:
    void send(uint8_t *data, uint64_t length);
    void receive();

    uint32_t status();
    void irq_reset();

    void startlink();
    bool start();

    E1000(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<E1000*> devices;

void init();
}