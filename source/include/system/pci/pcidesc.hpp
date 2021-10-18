#pragma once

#include <stdint.h>

extern char *device_classes[20];

char *getvendorname(uint16_t vendorid);
char *getdevicename(uint16_t vendorid, uint16_t deviceid);
char *getsubclassname(uint8_t classcode, uint8_t subclasscode);
char *getprogifname(uint8_t classcode, uint8_t subclasscode, uint8_t progif);