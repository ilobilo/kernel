// Copyright (C) 2021-2022  ilobilo

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
ps2::mousestate getmousestate()
{
    if (buttons & 0x20) return ps2::mousestate::PS2_LEFT;
    if (buttons & 0x10) return ps2::mousestate::PS2_RIGHT;
    if (buttons & 0x08) return ps2::mousestate::PS2_MIDDLE;
    return ps2::mousestate::PS2_NONE;
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
    ps2::mousepos.X = (cmd.bx * framebuffer::main_frm->width) / 0xFFFF;
    ps2::mousepos.Y = (cmd.cx * framebuffer::main_frm->height) / 0xFFFF;

    ps2::mouseclear();

    static bool circle = false;
    switch(ps2::getmousestate())
    {
        case ps2::PS2_LEFT:
            if (circle) framebuffer::drawfilledcircle(ps2::mousepos.X, ps2::mousepos.Y, 5, 0xff0000);
            else framebuffer::drawfilledrectangle(ps2::mousepos.X, ps2::mousepos.Y, 10, 10, 0xff0000);
            break;
        case ps2::PS2_MIDDLE:
            if (circle) circle = false;
            else circle = true;
            break;
        case ps2::PS2_RIGHT:
            if (circle) framebuffer::drawfilledcircle(ps2::mousepos.X, ps2::mousepos.Y, 5, 0xdd56f5);
            else framebuffer::drawfilledrectangle(ps2::mousepos.X, ps2::mousepos.Y, 10, 10, 0xdd56f5);
            break;
        default:
            break;
    }

    ps2::mousedraw();
    ps2::mouseposold = ps2::mousepos;
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

    ps2::mousevmware = true;
}

void mouse_relative()
{
    CMD cmd;

    cmd.bx = ABSPOINTER_RELATIVE;
    cmd.command = CMD_ABSPOINTER_COMMAND;
    send(cmd);

    ps2::mousevmware = false;
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