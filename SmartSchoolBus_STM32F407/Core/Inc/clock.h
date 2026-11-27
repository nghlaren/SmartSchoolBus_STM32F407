#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct { unsigned year,month,day,hour,minute,second; } BusTime;
unsigned clock_days(unsigned year,unsigned month);
bool clock_valid(const BusTime *t);
bool clock_init(void);
bool clock_get(BusTime *t);
bool clock_set(const BusTime *t);
bool clock_is_lsi(void);
void clock_stamp(char out[20]);
