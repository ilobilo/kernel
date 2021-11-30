// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/acpi/acpi.hpp>
#include <system/acpi/madt.hpp>
#include <lib/mmio.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::madt {

bool initialised = false;

Vector<MADTlapic*> MADTLapics;
Vector<MADTioapic*> MADTIOApics;
Vector<MADTiso*> MADTIsos;
Vector<MADTnmi*> MADTNmis;

uintptr_t lapic_addr = 0;

uintptr_t getLapicAddr()
{
    return lapic_addr;
}

void init()
{
    serial::info("Initialising MADT");

    if (initialised)
    {
        serial::warn("MADT has already been initialised!\n");
        return;
    }

    MADTLapics.init(1);
    MADTIOApics.init(1);
    MADTIsos.init(1);
    MADTNmis.init(1);

    lapic_addr = acpi::madthdr->local_controller_addr;
    
    for (uint8_t *madt_ptr = (uint8_t*)acpi::madthdr->madt_entries_begin; (uintptr_t)madt_ptr < (uintptr_t)acpi::madthdr + acpi::madthdr->sdt.length; madt_ptr += *(madt_ptr + 1))
    {
        switch (*(madt_ptr))
        {
            case 0:
                MADTLapics.push_back((MADTlapic*)madt_ptr);
                break;
            case 1:
                MADTIOApics.push_back((MADTioapic*)madt_ptr);
                break;
            case 2:
                MADTIsos.push_back((MADTiso*)madt_ptr);
                break;
            case 4:
                MADTNmis.push_back((MADTnmi*)madt_ptr);
                break;
            case 5:
                lapic_addr = QWORD_PTR(madt_ptr + 4);
                break;
            default: serial::err("sfgd");
        }
    }

    serial::newline();
    initialised = true;
}
}