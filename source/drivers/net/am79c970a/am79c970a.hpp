// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/net/cardmgr/cardmgr.hpp>
#include <system/pci/pci.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system;

namespace kernel::drivers::net::am79c970a {

#define AM79C970A_NUM_RX_DESC 32
#define AM79C970A_NUM_TX_DESC 8

#define AM79C970A_LOG2_NUM_RX_DESC 5
#define AM79C970A_LOG2_NUM_TX_DESC 3

#define DESC_LEN 16
#define BUFF_LEN 1544

struct [[gnu::packed]] initBlock
{
    uint16_t mode;
    uint8_t reserved1 : 4;
    uint8_t rxdescs : 4;
    uint8_t reserved2 : 4;
    uint8_t txdescs : 4;
    uint8_t mac[6];
    uint16_t reserved3;
    uint64_t logaddr;
    uint32_t rxdescaddr;
    uint32_t txdescaddr;
};

class AM79C970A : public cardmgr::NetCard
{
    private:
    pci::pcidevice_t *pcidevice;
    volatile lock_t lock;

    uint8_t BARType = 0;
    uint16_t IOBase = 0;

    initBlock *initblock;

    uint8_t *rxdesc;
    uint8_t *txdesc;

    uint8_t *rxbuffers[AM79C970A_NUM_RX_DESC];
    uint8_t *txbuffers[AM79C970A_NUM_TX_DESC];

    uint16_t rxcurr = 0;
    uint16_t txcurr = 0;

    uint16_t rxnext();
    uint16_t txnext();

    int owner(uint8_t *desc, size_t i);
    void initDE(size_t i, bool rx);

    void read_mac();

    void outrap32(uint32_t val);

    uint32_t incsr32(uint32_t csr_no);
    void outcsr32(uint32_t csr_no, uint32_t val);

    uint32_t inbcr32(uint32_t bcr_no);
    void outbcr32(uint32_t bcr_no, uint32_t val);

    public:
    uint8_t MAC[6];

    void send(void *data, uint64_t length);
    void recive();

    uint16_t status();
    void irq_reset(uint32_t status);

    void reset();
    void start();

    AM79C970A(pci::pcidevice_t *pcidevice);
};

extern bool initialised;
extern vector<AM79C970A*> devices;

void init();
}