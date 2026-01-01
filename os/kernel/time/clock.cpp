#include "clock.h"
#include "../drivers/rtc.h"

static int timezone_offset = 0; // UTC by default

void time_init()
{
    timezone_offset = 0;
}

void time_set_timezone(int offset_hours)
{
    timezone_offset = offset_hours;
}

int time_get_timezone()
{
    return timezone_offset;
}

static void apply_timezone(struct datetime* t)
{
    int h = (int)t->hour + timezone_offset;

    if (h < 0) {
        h += 24;
        t->day--;
    }
    if (h >= 24) {
        h -= 24;
        t->day++;
    }

    t->hour = (uint8_t)h;
}

void time_get_utc(struct datetime* out)
{
    rtc_time rtc;
    rtc_read(&rtc);

    out->year   = rtc.year;
    out->month  = rtc.month;
    out->day    = rtc.day;
    out->hour   = rtc.hour;
    out->minute = rtc.minute;
    out->second = rtc.second;
}

void time_get_local(struct datetime* out)
{
    time_get_utc(out);
    apply_timezone(out);
}
