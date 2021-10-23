#pragma once

namespace kernel::system::sched::rtc {

int year(), month(), day(), hour(), minute(), second();

void sleep(int sec);

char *getTime();
}