#include "kernel.h"
#include "dlist.h"
#include "main.h"
#include "limits.h"
#include "kernel_hwdep.h"
#include "interProcessCommunication.h"
#include "timerfunc.h"



//Administration prototypes-----------------------------
exception init_kernel(void);
exception create_task(void(*task_body)(),uint deadline);
void run(void);
void terminate(void);
void setKernelMode(uint);
uint getKernelMode(void);
void *allocateMem(uint);
void freeMem(void *);
//------------------------------------------------------

uint kernelMode;

//Administration functions
exception init_kernel(){
  
  set_ticks(0);
  
  timerlist = (list *)create_list();
  if(timerlist == NULL) return FAIL;
  
  waitinglist = (list *)create_list();
  if(waitinglist == NULL) return FAIL;
  
  readylist = (list *)create_list();
  if(readylist == NULL) return FAIL;
  
  setKernelMode(INIT);
  
  return OK;
  
}
exception create_task(void(*task_body)(),uint deadline){
  
  TCB * task = (TCB *) allocateMem(1*sizeof(TCB));
  if(task == NULL) return FAIL;
  
  task->DeadLine = deadline;
  task->PC = task_body;
  task->SPSR = 0;
  task->SP = &(task->StackSeg[STACK_SIZE-1]);
  
  //kernel är i startup läge
  if(getKernelMode() == INIT){
    
    listobj * tmpListObj = create_TCBlistobj(task);
    if(tmpListObj == NULL) return FAIL;
    
    insert(readylist, tmpListObj,1);
    
    return OK;
    
  }
  //kernel är i kör läge
  else{
    
    volatile int first = 1;
    
    //isr_off();
    SaveContext();
    
    if(first){
      
      first = 0;
      
      listobj * tmpListObj = create_TCBlistobj(task);
      
      if(tmpListObj == NULL) return FAIL;
      
      insert(readylist, tmpListObj,1);
      
      Running = readylist->pHead->pNext->pTask;
      
      LoadContext();
      
    }
    
  }
  
  return OK;
  
}
void run(void){
  
  Running = readylist->pHead->pNext->pTask;
  
  setKernelMode(RUNNING);
  
  LoadContext();
  
}
void terminate(void){
  
  //ta bort första objekt från readylist
  listobj *obj = readylist->pHead->pNext;
  
  readylist->pHead->pNext = obj->pNext;
  obj->pNext->pPrevious = readylist->pHead;
  
  freeMem(obj->pTask);
  freeMem(obj);
  
  //ladda nästa task som ska köras
  Running = readylist->pHead->pNext->pTask;
  
  LoadContext();
  
}
void setKernelMode(uint mode){
  
  kernelMode = mode;
  
}
uint getKernelMode(void){
  
  return kernelMode;
  
}
void *allocateMem(uint size){
  
  void *returnValue = malloc(size);
  
  return returnValue;
  
}
void freeMem(void *ptr){
  
  free(ptr);
  
  ptr = NULL;
  
}
//------------------------