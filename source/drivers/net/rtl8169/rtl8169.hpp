// // Copyright (C) 2021  ilobilo

// #pragma once

// #include <drivers/net/nicmgr/nicmgr.hpp>
// #include <system/pci/pci.hpp>
// #include <lib/lock.hpp>
// #include <stdint.h>

// using namespace kernel::system;

// namespace kernel::drivers::net::rtl8169 {

// #define RTL8169_OWN (1 << 31)
// #define RTL8169_EOR (1 << 30)
// #define RTL8169_FFR (1 << 29)
// #define RTL8169_LFR (1 << 28)

// #define RTL8169_NUM_RX_DESC 32
// #define RTL8169_NUM_TX_DESC 8

// #define RTL8169_RX_BUFF_SIZE 8192

// struct [[gnu::packed]] Desc
// {
//     uint32_t command;
//     uint32_t vlan;
//     uint64_t buffer;
// };

// class RTL8169 : public nicmgr::NIC
// {
//     private:
//     pci::pcidevice_t *pcidevice;
//     volatile lock_t lock;

//     uint16_t IOBase = 0;
//     uint8_t BARType = 0;

//     Desc *rxdescs[RTL8169_NUM_RX_DESC];
//     Desc *txdescs[RTL8169_NUM_TX_DESC];

//     uint16_t rxcurr = 0;
//     uint16_t txcurr = 0;

//     void rxinit();
//     void txinit();

//     public:
//     void send(void *data, uint64_t length);
//     void receive();

//     void reset();
//     void start();

//     uint16_t status();
//     void irq_reset(uint16_t status);

//     void read_mac();

//     RTL8169(pci::pcidevice_t *pcidevice);
// };

// extern bool initialised;
// extern vector<RTL8169*> devices;

// void init();
// }