#pragma once
#include <stdint.h>
#include <stdbool.h>
#define STUDENTS 32
typedef enum { IDLE, PICKUP, DROP, MISSING, SWEEP } Phase;
typedef enum { REGISTER, START, MODE, BOARD, EXIT_BUS, END, REAR, REPLACE, CLEAR_CARDS, MANUAL_EXIT, ACTIONS } Action;
typedef struct { char uid[STUDENTS][9]; uint8_t status[STUDENTS]; Phase phase; unsigned trip; } Trip;
extern char names[STUDENTS][11];
extern unsigned roster_count, roster_order[STUDENTS];
bool roster_active(unsigned slot);
bool roster_load(const Trip *t);
bool any_registered(const Trip *t);
extern const char *const events[ACTIONS];
int onboard(const Trip *t);
int student(const Trip *t,const char *uid);
bool transition(Trip *t, Action a,int slot,const char *uid);
bool fresh_rear_press(Phase phase,uint32_t down,uint32_t sweep_start);

