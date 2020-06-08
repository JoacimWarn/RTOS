// main.c
#include "kernel.h"
#include "limits.h"
#include "interProcessCommunication.h"
#include "timerfunc.h"


TCB * Running;
list *readylist;
list *timerlist;
list *waitinglist;


void idle_task(void);
void task1(void);
void task2(void);
void task3(void);


#define TEST_PATTERN_1 0xAA
#define TEST_PATTERN_2 0x55
mailbox *mb;
int nTest1 = 0;
int nTest2 = 0;
int nTest3 = 0;



int main(void)
{
  
  if(init_kernel() != OK) while(1);
  
  if(create_task(&idle_task,UINT_MAX) != OK) while(1);
  
  if(create_task(&task1,2000) != OK) while(1);
  
  if(create_task(&task2,4000) != OK) while(1);
  
  //if(create_task(&task3,3000) != OK) while(1);
  
  mb = create_mailbox(1,sizeof(int));
  
  if(mb == NULL) while(1);
  
  run();
  
  return 1;
  
}

void task1(void)
{
  int nData_t1 = TEST_PATTERN_1;
  wait(10); /* task2 börjar köra */
  if( no_messages(mb) != 1 )
   terminate(); /* ERROR */
  if(send_wait(mb,&nData_t1) == DEADLINE_REACHED)
   terminate(); /* ERROR */
  wait(10); /* task2 börjar köra */
  /* start test 2 */
  nData_t1 = TEST_PATTERN_2;
  if(send_wait(mb,&nData_t1) == DEADLINE_REACHED)
   terminate(); /* ERROR */
  wait(10); /* task2 börjar köra */
  /* start test 3 */
  if(send_wait(mb,&nData_t1)==DEADLINE_REACHED) {
   if( no_messages(mb) != 0 )
   terminate(); /* ERROR */
   nTest3 = 1;
   if (nTest1*nTest2*nTest3) {
     printf("hey");
  /* Blinka lilla lysdiod */
   /* Test ok! */
   }
   terminate(); /* PASS, no receiver */
   }
   else
   {
   terminate(); /* ERROR */
   }
}


void task2(void)
{
  int nData_t2 = 0;
  if(receive_wait(mb,&nData_t2) ==
  DEADLINE_REACHED) /* t1 kör nu */
   terminate(); /* ERROR */
  if( no_messages(mb) != 0 )
  terminate(); /* ERROR */
  if (nData_t2 == TEST_PATTERN_1) nTest1 = 1;
  wait(20); /* t1 kör nu */
  /* start test 2 */
  if( no_messages(mb) != 1 )
  terminate(); /* ERROR */
  if(receive_wait(mb,&nData_t2) ==
  DEADLINE_REACHED) /* t1 kör nu */
  terminate(); /* ERROR */
  if( no_messages(mb) != 0 )
  terminate(); /* ERROR */
  if (nData_t2 == TEST_PATTERN_2) nTest2 = 1;
  /* Start test 3 */
  terminate();
}


void task3(void){
  
  wait(1995);
  
  terminate();
  
}


void idle_task(void){
  
  while(1){
    
    TimerInt();
    
    //Timer0Int();
    
  }
  
  
}

