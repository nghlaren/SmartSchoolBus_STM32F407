#pragma once
#include "trip.h"
bool journal_open(Trip *t);
bool journal_append(const Trip *next,Action a,int slot,const char *uid,uint32_t ms);
