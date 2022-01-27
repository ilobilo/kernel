// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

uint8_t flip8(uint8_t num, size_t bits);
uint16_t flip16(uint16_t num);
uint32_t flip32(uint32_t num);

uint8_t htonb(uint8_t num, size_t bits);
uint8_t ntohb(uint8_t num, size_t bits);

uint16_t htons(uint16_t num);
uint16_t ntohs(uint16_t num);

uint32_t htonl(uint32_t num);
uint32_t ntohl(uint32_t num);

uint16_t checksum(void *addr, size_t size, size_t start = 0); 