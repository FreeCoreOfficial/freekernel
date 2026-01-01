#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct datetime {
    uint16_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

void time_init();
void time_set_timezone(int offset_hours);
int  time_get_timezone();

void time_get_utc(struct datetime* out);
void time_get_local(struct datetime* out);

#ifdef __cplusplus
}
#endif
