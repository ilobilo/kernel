// Copyright (C) 2021  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/vmware/vmware.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;

namespace kernel::drivers::vmware {

bool initialised = false;

void send(CMD &cmd)
{
    cmd.magic = VMWARE_MAGIC;
    cmd.port = VMWARE_PORT;
    asm volatile("in %%dx, %0" : "+a"(cmd.ax), "+b"(cmd.bx), "+c"(cmd.cx), "+d"(cmd.dx), "+S"(cmd.si), "+D"(cmd.di));
}

bool is_vmware_backdoor()
{
    CMD cmd;
    cmd.bx = ~VMWARE_MAGIC;
    cmd.command = CMD_GETVERSION;
    send(cmd);

    if (cmd.bx != VMWARE_MAGIC || cmd.ax == 0xFFFFFFFF) return false;
    return true;
}

int buttons;
mouse::mousestate getmousestate()
{
    if (buttons & 0x20) return mouse::mousestate::ps2_left;
    if (buttons & 0x10) return mouse::mousestate::ps2_right;
    if (buttons & 0x08) return mouse::mousestate::ps2_middle;
    return mouse::mousestate::ps2_none;
}

void handle_mouse()
{
    CMD cmd;

    cmd.bx = 0;
    cmd.command = CMD_ABSPOINTER_STATUS;
    send(cmd);

    if (cmd.ax == 0xFFFF0000)
    {
        mouse_relative();
        mouse_absolute();
        return;
    }

    if ((cmd.ax & 0xFFFF) < 4) return;

    cmd.bx = 4;
    cmd.command = CMD_ABSPOINTER_DATA;
    send(cmd);

    buttons = (cmd.ax & 0xFFFF);
    mouse::pos.X = (cmd.bx * framebuffer::frm_width) / 0xFFFF;
    mouse::pos.Y = (cmd.cx * framebuffer::frm_height) / 0xFFFF;

    mouse::clear();

    static bool circle = false;
    switch(mouse::getmousestate())
    {
        case mouse::ps2_left:
            if (circle) framebuffer::drawfilledcircle(mouse::pos.X, mouse::pos.Y, 5, 0xff0000);
            else framebuffer::drawfilledrectangle(mouse::pos.X, mouse::pos.Y, 10, 10, 0xff0000);
            break;
        case mouse::ps2_middle:
            if (circle) circle = false;
            else circle = true;
            break;
        case mouse::ps2_right:
            if (circle) framebuffer::drawfilledcircle(mouse::pos.X, mouse::pos.Y, 5, 0xdd56f5);
            else framebuffer::drawfilledrectangle(mouse::pos.X, mouse::pos.Y, 10, 10, 0xdd56f5);
            break;
        default:
            break;
    }

    mouse::draw();

    mouse::posold = mouse::pos;
}

void mouse_absolute()
{
    CMD cmd;

    cmd.bx = ABSPOINTER_ENABLE;
    cmd.command = CMD_ABSPOINTER_COMMAND;
    send(cmd);

    cmd.bx = 0;
    cmd.command = CMD_ABSPOINTER_STATUS;
    send(cmd);

    cmd.bx = 1;
    cmd.command = CMD_ABSPOINTER_DATA;
    send(cmd);

    cmd.bx = ABSPOINTER_ABSOLUTE;
    cmd.command = CMD_ABSPOINTER_COMMAND;
    send(cmd);

    mouse::vmware = true;
}

void mouse_relative()
{
    CMD cmd;

    cmd.bx = ABSPOINTER_RELATIVE;
    cmd.command = CMD_ABSPOINTER_COMMAND;
    send(cmd);

    mouse::vmware = false;
}

void init()
{
    log("Initialising VMWare tools");

    if (initialised)
    {
        warn("VMWare tools have already been initialised!\n");
        return;
    }
    if (!is_vmware_backdoor())
    {
        error("VMWare backdoor is not available!\n");
        return;
    }

    mouse_absolute();

    serial::newline();
    initialised = true;
}
}