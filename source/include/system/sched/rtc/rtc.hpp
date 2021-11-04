// Copyright (C) 2021  ilobilo

#pragma once

namespace kernel::system::sched::rtc {

int year(), month(), day(), hour(), minute(), second();

int time();

void sleep(int sec);

char *getTime();
}