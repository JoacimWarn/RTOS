// dlist.c


#include "dlist.h"
#include "utest.h"
#include "kernel.h"
#include "main.h"
#include "limits.h"
#include "kernel_hwdep.h"


list * create_list(){
  
  list * mylist = (list *)allocateMem(sizeof(list));
  if (mylist == NULL){
    return NULL;
  }
  
  mylist->pHead = (listobj *)allocateMem(sizeof(listobj));
  if (mylist->pHead == NULL){
    freeMem(mylist);
    return NULL;
  }
  
  mylist->pTail = (listobj *)allocateMem(sizeof(listobj));
  if (mylist->pTail == NULL){
    freeMem(mylist->pHead);
    freeMem(mylist);
    return NULL;
  }
  
  mylist->pHead->pPrevious = mylist->pHead;
  mylist->pHead->pNext = mylist->pTail;
  mylist->pTail->pPrevious = mylist->pHead;
  mylist->pTail->pNext = mylist->pTail;
  
  return mylist;
  
}


listobj * create_listobj(int num){
  
  listobj * myobj = (listobj *)allocateMem(sizeof(listobj));
  
  if (myobj == NULL) return NULL;
  
  myobj->nTCnt = num;
  
  return (myobj);
  
}


listobj * create_TCBlistobj(TCB *TCBObj){
  
  listobj * myobj = (listobj *) allocateMem(1*sizeof(listobj));
  
  if(myobj == NULL) return NULL;
  
  myobj->pTask = TCBObj;
  myobj->pMessage = NULL;
  
  return myobj;
  
}


void moveToFirst(list *src, list *dst, int short sort){
  
  //hämta första elementet
  //listobj *first = src->pHead->pNext;
  listobj * first = extract(src->pHead->pNext);
  
  insert(dst,first,sort);
  
}


//insättningsfunktion med sorterings möjlighet
void insert(list * dst, listobj * pObj, int short sort){
  
  if(dst == readylist){
    printf("readylist\n");
  }
  else if(dst == waitinglist){
    printf("waitinglist\n");
  }
  
  //listan var tom, sätt in objektet på första positionen
  if(isEmptyList(dst)){
    
    dst->pHead->pNext = pObj;
    dst->pTail->pPrevious = pObj;
    
    pObj->pPrevious = dst->pHead;
    pObj->pNext = dst->pTail;
    
  }
  //listan var inte tom, leta upp rätt plats
  else{
    
    listobj *iterator = dst->pHead->pNext;
    
    while(iterator != dst->pTail){
      
      //insättning m.h.a deadline
      if(sort == 1){
        
        if((pObj->pTask->DeadLine) < (iterator->pTask->DeadLine)) break;
        
      }
      //insättning m.h.a nTCnt
      else{
        
        if(pObj->nTCnt < iterator->nTCnt) break;
        
      }
      
      iterator = iterator->pNext;
      
    }
    
    //knyt om förindelserna
    pObj->pNext = iterator;
    pObj->pPrevious = iterator->pPrevious;
    iterator->pPrevious->pNext = pObj;
    iterator->pPrevious = pObj;
    
  } 
  
}


//Sök igenom waitinglist
void searchWL(){
  
  uint tick = ticks();
  
  listobj * iterator = waitinglist->pHead->pNext;
  
  while(iterator != waitinglist->pTail){
    
    if(iterator->pTask->DeadLine <= tick){
      
      iterator = iterator->pNext;
      
      listobj * first = extract(waitinglist->pHead->pNext);
      insert(readylist,first,1);
      
      Running = readylist->pHead->pNext->pTask;
      
    }
    
    else break;
    
  }
  
}


//Sök igenom timerlist
void searchTL(){
  
  uint tick = ticks();
  
  listobj * iterator = timerlist->pHead->pNext;
  
  while(iterator != timerlist->pTail){
    
    if(iterator->nTCnt <= tick){
      
      iterator = iterator->pNext;
      
      listobj * first = extract(timerlist->pHead->pNext);
      insert(readylist,first,1);
      
      Running = readylist->pHead->pNext->pTask;
      
    }
    
    else break;
    
  }
  
}


//extrahera objekt ur lista
listobj * extract(listobj *pObj){
  
  pObj->pPrevious->pNext = pObj->pNext;
  pObj->pNext->pPrevious = pObj->pPrevious;
  pObj->pNext = NULL;
  pObj->pPrevious = NULL;
  
  return (pObj);
  
}

