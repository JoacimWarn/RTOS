#ifndef _TIMERFUNC_H_
#define _TIMERFUNC_H_
#include "kernel.h"

//Timing prototypes-----
void TimerInt(void);
exception wait(uint);
void set_ticks(uint);
uint ticks(void);
uint deadline(void);
void set_deadline(uint);
//----------------------

#endif