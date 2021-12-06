// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::pci {

extern const char *device_classes[20];

const char *getvendorname(uint16_t vendorid);
const char *getdevicename(uint16_t vendorid, uint16_t deviceid);
const char *getsubclassname(uint8_t classcode, uint8_t subclasscode);
const char *getprogifname(uint8_t classcode, uint8_t subclasscode, uint8_t progif);
}