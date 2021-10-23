#include <drivers/display/serial/serial.hpp>
#include <system/cpu/gdt/gdt.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::cpu::gdt {

__attribute__((aligned(0x1000)))
GDT DefaultGDT = {
    {0, 0, 0, 0x00, 0x00, 0},
    {0xffff, 0, 0, 0x9a, 0x80, 0},
    {0xffff, 0, 0, 0x92, 0x80, 0},
    {0xffff, 0, 0, 0x9a, 0xcf, 0},
    {0xffff, 0, 0, 0x92, 0xcf, 0},
    {0, 0, 0, 0x9a, 0xa2, 0},
    {0, 0, 0, 0x92, 0xa0, 0},
};

bool initialised = false;

void init()
{
    serial::info("Initialising GDT\n");

    if (initialised)
    {
        serial::info("GDT has already been initialised!\n");
        return;
    }

    GDTDescriptor gdtDescriptor;
    gdtDescriptor.Size = sizeof(GDT) - 1;
    gdtDescriptor.Offset = (uint64_t)&DefaultGDT;
    LoadGDT(&gdtDescriptor);

    initialised = true;
}
}