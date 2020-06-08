#include "kernel.h"
#include "main.h"
#include "dlist.h"


//Timing prototypes-----
void TimerInt(void);
exception wait(uint);
void set_ticks(uint);
uint ticks(void);
uint deadline(void);
void set_deadline(uint);
//----------------------

uint nrOfTicks;

//Timing functions------------
void TimerInt(void)
{
  
  SaveContext();
  
  uint tick = ticks();
  tick++;
  set_ticks(tick);
  
  
  //sök i waitinglist
  searchWL();
  
  //sök i timerlist
  searchTL();
  
  
  LoadContext();
  
}
exception wait(uint nTicks){
  
  volatile int first = 1;
  
  //isr_off();
  SaveContext();
  
  if(first){
    
    first = 0;
    
    //placera running task i timerlist
    readylist->pHead->pNext->nTCnt = ticks() + nTicks;
    
    listobj * first = extract(readylist->pHead->pNext);
    insert(timerlist,first,-1);
    
    Running = readylist->pHead->pNext->pTask;
    
    LoadContext();
    
  }
  else{
    
    if(Running->DeadLine >= ticks()) return DEADLINE_REACHED;
    
    else return OK;
    
  }
  
  return OK;
  
}
void set_ticks(uint nTicks){
  
  nrOfTicks = nTicks;
  
}
uint ticks(){
  
  return nrOfTicks;
  
}
uint deadline(void){
  
  return Running->DeadLine;
  
}
void set_deadline(uint deadline){
  
  volatile int first = 1;
  
  SaveContext();
  
  if(first){
    
    first = 0;
    Running->DeadLine = deadline;
    
    //flytta först
    listobj * first = extract(readylist->pHead->pNext);
    insert(readylist,first,1);
    
    Running = readylist->pHead->pNext->pTask;
    
    LoadContext();
    
  }
  
}
//-------------------------------