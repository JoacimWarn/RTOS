#include "kernel.h"
#include "limits.h"
#include "dlist.h"


uint nrOfTicks;



//Administration prototypes-----------------------------
int init_kernel(void);
exception create_task(void(*task_body)(),uint deadline);
void run(void);
void terminate(void);
//------------------------------------------------------


//Timing prototypes-----
exception wait(uint);
void set_ticks(uint);
uint ticks(void);
uint deadline(void);
void set_deadline(uint);
//----------------------


//Communication prototypes-------------------------------
mailbox* create_mailbox(uint , uint);
exception remove_mailbox(mailbox* );
int no_messages(mailbox* );
exception send_wait(mailbox* , void*);
exception receive_wait(mailbox* , void*);
exception send_no_wait(mailbox* , void*);
exception receive_no_wait(mailbox* , void* );
//-------------------------------------------------------


//Task1-Task2----
void task1(void);
void task2(void);
//---------------


//Task1-Task2----
void task1(void){
  
  create_task(task2,200);
  //send_wait(mb,msg);
  //send_wait(mb,msg);
  //terminate();
  
}
void task2(void){
  
  //receive_no_wait(mb,msg);
  //wait(99);
  //terminate();
  
}
//---------------


//Administration functions
int init_kernel(){
  
  set_ticks(0);
  
  readylist = create_list();
  waitinglist = create_list();
  timinglist = create_list();
  
  if((readylist || waitinglist || timinglist) == NULL) return FAIL;
  
  if((create_task(task1,UINT_MAX)) == FAIL) return FAIL;;
  
  return OK;
  
}
exception create_task(void(*task_body)(),uint deadline){
  
  TCB * task = calloc(1,sizeof(TCB));
  
  if(task == NULL) return FAIL;
  
  task->DeadLine = deadline;
  task->PC = task_body;
  task->SP = task->StackSeg;
  
  //Startup mode
  if(nrOfTicks <= 0){
    
    listobj * obj = NULL;
    obj = create_listobj(deadline);
    insert(readylist,obj);
    
    return OK;
    
  }
  //Running mode
  else{
    
    isr_off();
    SaveContext();
    
    
    
  }
  
  //TCB->DeadLine = deadline;
  
  return OK;
  
}
void run(void){
  
  //timer0_start();
  set_isr(30);
  
  isr_on();
  
  LoadContext();
  
}
//------------------------


//Timing functions------------
void set_ticks(uint nTicks){
  
  nrOfTicks = nTicks;
  
}


uint ticks(){
  
  return nrOfTicks;
  
}
//----------------------------


//Communication functions--------



//-------------------------------



