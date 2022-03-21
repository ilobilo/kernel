// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/string.hpp>
#include <cstdint>

namespace kernel::system::pci {

extern const char *device_classes[20];

string getvendorname(uint16_t vendorid);
string getdevicename(uint16_t vendorid, uint16_t deviceid);
string getsubclassname(uint8_t classcode, uint8_t subclasscode);
string getprogifname(uint8_t classcode, uint8_t subclasscode, uint8_t progif);
}