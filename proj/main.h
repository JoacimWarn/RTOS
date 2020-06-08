#ifndef MAIN_H
#define MAIN_H

void idle_task(void);
void task1(void);
void task2(void);

extern TCB * Running;
extern list *readylist;
extern list *timerlist;
extern list *waitinglist;

#endif