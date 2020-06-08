// main.c
#include "kernel.h"


TCB * Running;
list * readylist;
list * waitinglist;
list * timinglist;


void TimerInt(void)
{
  
  set_ticks(ticks()+1);
  
}


int main(void)
{
  
  init_kernel();
  //mb * create_mailbox(1,sizeof(int));
  //mailbox* mb = create_mailbox(1,sizeof(int));
  
  run();
  
  /*
  list * create_list();
  listobj * create_listobj(int num);
  
  void insert(list * mylist, listobj * pObj);
  listobj * extract(listobj * pObj);
  */
  
  //printf("hey");
  
    while(1);
    
  return 1;
  
}


