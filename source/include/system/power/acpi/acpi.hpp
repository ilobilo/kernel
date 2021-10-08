#pragma once

#include <stdint.h>

struct RSDP
{
    unsigned char signature[8];
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t revision;
    uint32_t rsdtaddr;
    uint32_t length;
    uint64_t xsdtaddr;
    uint8_t extchksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct sdt_header
{
    unsigned char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t oemtableid[8];
    uint32_t oemrevision;
    uint32_t creatid;
    uint32_t creatrevision;
} __attribute__((packed));

struct mcfg_header
{
    sdt_header header;
    uint64_t reserved;
} __attribute__((packed));

struct deviceconfig
{
    uint64_t baseaddr;
    uint16_t pciseggroup;
    uint8_t startbus;
    uint8_t endbus;
    uint32_t reserved;
} __attribute__((packed));

extern bool acpi_initialised;

extern bool use_xstd;
extern RSDP *rsdp;

extern mcfg_header *mcfg;

void ACPI_init();

void *findtable(sdt_header *sdthdr, char *signature);
