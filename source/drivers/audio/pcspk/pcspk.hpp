// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::drivers::audio::pcspk {

enum notes
{
    A0 = 28,
    AS0 = 29,
    B0 = 31,
    
    C1 = 33,
    CS1 = 35,
    D1 = 37,
    DS1 = 39,
    E1 = 41,
    F1 = 44,
    FS1 = 46,
    G1 = 49,
    GS1 = 52,
    A1 = 55,
    AS1 = 58,
    B1 = 62,
    
    C2 = 65,
    CS2 = 69,
    D2 = 73,
    DS2 = 78,
    E2 = 82,
    F2 = 87,
    FS2 = 92,
    G2 = 98,
    GS2 = 104,
    A2 = 110,
    AS2 = 117,
    B2 = 123,
    
    C3 = 131,
    CS3 = 139,
    D3 = 147,
    DS3 = 156,
    E3 = 165,
    F3 = 175,
    FS3 = 185,
    G3 = 196,
    GS3 = 208,
    A3 = 220,
    AS3 = 233,
    B3 = 247,

    C4 = 262,
    CS4 = 277,
    D4 = 294,
    DS4 = 311,
    E4 = 330,
    F4 = 349,
    FS4 = 370,
    G4 = 392,
    GS4 = 415,
    A4 = 440,
    AS4 = 466,
    B4 = 494,

    C5 = 523,
    CS5 = 554,
    D5 = 587,
    DS5 = 622,
    E5 = 659,
    F5 = 698,
    FS5 = 739,
    G5 = 783,
    GS5 = 830,
    A5 = 880,
    AS5 = 932,
    B5 = 987,

    C6 = 1046,
    CS6 = 1108,
    D6 = 1174,
    DS6 = 1244,
    E6 = 1318,
    F6 = 1396,
    FS6 = 1479,
    G6 = 1567,
    GS6 = 1661,
    A6 = 1760,
    AS6 = 1864,
    B6 = 1975,

    DefaultNote = 800,
};

enum durations
{
    Whole = 1600,
    Half = Whole / 2,
    Quarter = Half / 2,
    Eighth = Quarter / 2,
    Sixteenth = Eighth / 2,
    
    DefaultDur = 200,
};

void play(uint64_t freq);

void stop();

void beep(uint64_t freq, uint64_t msec);
}