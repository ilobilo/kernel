#pragma once

int RTC_year(), RTC_month(), RTC_day(), RTC_hour(), RTC_minute(), RTC_second();

void RTC_sleep(int sec);

char *RTC_GetTime();