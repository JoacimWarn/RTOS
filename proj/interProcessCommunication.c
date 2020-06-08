#include "kernel.h"
#include "dlist.h"
#include "main.h"


//Communication prototypes-------------------------------
mailbox* create_mailbox(uint , uint);
exception remove_mailbox(mailbox* );
exception send_wait(mailbox* , void*);
void removeFirstMsg(mailbox *);
int no_messages(mailbox* );
exception receive_wait(mailbox* , void*);
exception send_no_wait(mailbox* , void*);
exception receive_no_wait(mailbox* , void* );
//-------------------------------------------------------


//Communication functions--------
mailbox* create_mailbox(uint nof_msg, uint size_of_msg){
  
  mailbox * mb = (mailbox*)allocateMem(sizeof(mailbox));
  if(mb == NULL) return FAIL;           //Finns inget minne kvar = null
  
  mb->pHead = (msg*)allocateMem(sizeof(msg));           //Allokera minne för huvudet
  
  if(mb->pHead == NULL){
    //finns inget minne kvar = null
    freeMem(mb);
    return FAIL;
    
  }
  
  mb->pTail = (msg*)allocateMem(sizeof(msg));           //allokera minne för svansen
  
  if(mb->pTail == NULL){
    //finns inget minne kvar = null
    freeMem(mb->pHead);
    freeMem(mb);
    return FAIL;
    
  }
  
  mb->pHead->pNext = mb->pTail;
  mb->pHead->pPrevious = mb->pHead;
  mb->pTail->pPrevious = mb->pHead;
  mb->pTail->pNext = mb->pTail;
  
  mb->nMaxMessages = nof_msg;
  mb->nDataSize = size_of_msg;
  
  mb->nMessages = 0;
  mb->nBlockedMsg = 0;
  
  return mb;
  
}
exception remove_mailbox(mailbox * mb){
  
  if(mb->nBlockedMsg == 0 && mb->nMessages == 0){
    
    freeMem(mb->pTail);
    freeMem(mb->pHead);
    freeMem(mb);
    
    return OK;
    
  }
  
  return NOT_EMPTY;
  
}
exception send_wait(mailbox* mailBox, void* Data){
  
  volatile int first = 1;
  
  //isr_off();
  SaveContext();
  
  if(first){
    
    first = 0;
    
    //Om mottagaren har ett meddelande som väntar
    if(no_messages(mailBox) != 0 && mailBox->pHead->pNext->Status == RECEIVER){
      
      //kopiera sändar data till mottagare
      memcpy(mailBox->pHead->pNext->pData,Data,mailBox->nDataSize);
      mailBox->pHead->pNext->pBlock->pMessage = NULL;
      
      
      //flytta till readylist
      listobj *obj = mailBox->pHead->pNext->pBlock;
      obj->pPrevious->pNext = obj->pNext;
      obj->pNext->pPrevious = obj->pPrevious;
      
      insert(readylist,obj,1);
      
      Running = readylist->pHead->pNext->pTask;
      
      //FIFO struktur -> ta bort första meddelandet i postlådan
      removeFirstMsg(mailBox);
      
    }
    //Om mottagaren/avsändaren inte har ett meddelande som väntar
    else{
      
      msg * tmpMsg = (msg *)allocateMem(sizeof(msg));
      if(tmpMsg == NULL) return FAIL;
      
      tmpMsg->pData = Data;
      tmpMsg->Status = SENDER;
      tmpMsg->pBlock = readylist->pHead->pNext;
      
      //associera meddelandet till objektet
      readylist->pHead->pNext->pMessage = tmpMsg;
      
      
      //sätt in blockerande meddelande i postlåda
      tmpMsg->pPrevious = mailBox->pTail->pPrevious;
      tmpMsg->pNext = mailBox->pTail;
      mailBox->pTail->pPrevious->pNext = tmpMsg;
      mailBox->pTail->pPrevious = tmpMsg;
      mailBox->nBlockedMsg = mailBox->nBlockedMsg + 1;
      
      //moveToFirst(readylist, waitinglist, 0);
      listobj * first = extract(readylist->pHead->pNext);
      insert(waitinglist,first,0);
      
      //uppdatera running pekare
      Running = readylist->pHead->pNext->pTask;
      
    }
    
    LoadContext();
    
    
  }
  //Inte första gången det körs
  else{
    
    if(ticks() >= Running->DeadLine){
      
      msg *tmpMsg = readylist->pHead->pNext->pMessage;
      
      //Ta bort association med lista och frigör sedan minnespositionen
      tmpMsg->pPrevious->pNext = tmpMsg->pNext;
      tmpMsg->pNext->pPrevious = tmpMsg->pPrevious;
      
      freeMem(tmpMsg);
      mailBox->nBlockedMsg = mailBox->nBlockedMsg - 1;
      
      readylist->pHead->pNext->pMessage = NULL;
      
      return DEADLINE_REACHED;
      
    }
    
    else return OK;
    
  }
  
  return OK;
  
}
exception receive_wait(mailbox *mBox, void *Data){
  
  volatile int first;
  
  first = 1;
  
  //isr_off();
  
  SaveContext();
  
  if(first){
    
    first = 0;
    
    //Om vi har meddelande som väntar och vi är sändare
    if(no_messages(mBox) != 0 && mBox->pHead->pNext->Status == SENDER){
      
      //Kopiera sändardata till mottagaren
      memcpy(Data, mBox->pHead->pNext->pData, mBox->nDataSize);
      
      //Om meddelandet är av blockeringstyp
      if(mBox->nBlockedMsg != 0){
        
        //Meddelandet var av blockeringstyp -> meddelandet ska flyttas från waitinglist till readylist
        //Ta bort association mellan tasken och meddelandet
        mBox->pHead->pNext->pBlock->pMessage = NULL;
        
        
        mBox->pHead->pNext->pBlock->pPrevious->pNext = mBox->pHead->pNext->pBlock->pNext;
        mBox->pHead->pNext->pBlock->pNext->pPrevious = mBox->pHead->pNext->pBlock->pPrevious;
        
        insert(readylist,mBox->pHead->pNext->pBlock,1);
        
        //move(mBox->pHead->pNext->pBlock,waitinglist, readylist, 1); <---------------------
        
        //Uppdatera running pekare
        Running = readylist->pHead->pNext->pTask;
        removeFirstMsg(mBox);
        
      }
      //Meddelandet är inte av blockeringstyp
      else{
        
        removeFirstMsg(mBox);
        
      }
      
    }
    //Vi har inte meddelande som väntar eller vi är mottagaren
    else{
      
      //Allokera meddelande struktur och sätt in meddelande i postlåda
      msg * tmpMsg = (msg *) allocateMem(sizeof(msg));
      if(tmpMsg == NULL) return FAIL;
      
      //Initiera meddelande data
      tmpMsg->pData = Data;
      tmpMsg->Status = RECEIVER;
      tmpMsg->pBlock = readylist->pHead->pNext;
      
      //Associera meddelandet till tasken
      readylist->pHead->pNext->pMessage = tmpMsg;
      
      //sätt in blockerande meddelande i postlåda
      tmpMsg->pPrevious = mBox->pTail->pPrevious;
      tmpMsg->pNext = mBox->pTail;
      mBox->pTail->pPrevious->pNext = tmpMsg;
      mBox->pTail->pPrevious = tmpMsg;
      mBox->nBlockedMsg = mBox->nBlockedMsg + 1;
      
      
      listobj * first = extract(readylist->pHead->pNext);
      insert(waitinglist,first,0);
      
      //Uppdatera running pekare
      Running = readylist->pHead->pNext->pTask;
      
    }
    
    LoadContext();          //här blev det konstigt!
    
  } 
  else{
    
    //Deadline för tasken som körs har uppnåtts
    if(ticks() >= Running->DeadLine){
      
      msg *tmpMsg = readylist->pHead->pNext->pMessage;
      
      //Ta bort association med lista och frigör sedan minnespositionen
      tmpMsg->pPrevious->pNext = tmpMsg->pNext;
      tmpMsg->pNext->pPrevious = tmpMsg->pPrevious;
      
      freeMem(tmpMsg);
      mBox->nBlockedMsg = mBox->nBlockedMsg - 1;
      
      readylist->pHead->pNext->pMessage = NULL;
      
      return DEADLINE_REACHED;
      
    }
    
    else return OK;
    
  }
  
  return FAIL;
  
}
exception send_no_wait(mailbox *mBox, void *Data){
  
  volatile int first = 1;
  
  //isr_off();
  
  SaveContext();
  
  if(first){
    
    first = 0;
    
    if(no_messages(mBox) != 0 && mBox->pHead->pNext->Status == RECEIVER){
      
      memcpy(mBox->pHead->pNext->pData, Data, mBox->nDataSize);
      //Detta är ett blockeringsmeddelande
      //Ta bort association mellan tasken och meddelandet
      mBox->pHead->pNext->pBlock->pMessage = NULL;
      
      mBox->pHead->pNext->pBlock->pPrevious->pNext = mBox->pHead->pNext->pBlock->pNext;
      mBox->pHead->pNext->pBlock->pNext->pPrevious = mBox->pHead->pNext->pBlock->pPrevious;
      
      insert(readylist,mBox->pHead->pNext->pBlock,1);
      
      //Uppdadetar running pekare
      Running = readylist->pHead->pNext->pTask;
      removeFirstMsg(mBox);
      
    }
    else{
      
      //Postlådan är full, ta bort ett meddelande
      if(mBox->nMaxMessages <= mBox->nMessages){
        
        //Ta bort association mellan tasken och meddelandet
        mBox->pHead->pNext->pBlock->pMessage = NULL;
        removeFirstMsg(mBox);
        
      }
      //Nytt meddelande
      msg * tmpMsg = (msg *) allocateMem(sizeof(msg));
      if(tmpMsg == NULL) return FAIL;
      
      tmpMsg->pData = allocateMem(mBox->nDataSize);
      if(tmpMsg->pData == NULL){
        
        freeMem(tmpMsg);
        return FAIL;
        
      }
      
      memcpy(tmpMsg->pData, Data, mBox->nDataSize);
      tmpMsg->Status = SENDER;
      tmpMsg->pBlock = NULL;
      
      //Sätt in meddelande i postlåda
      tmpMsg->pPrevious = mBox->pTail->pPrevious;
      tmpMsg->pNext = mBox->pTail;
      mBox->pTail->pPrevious->pNext = tmpMsg;
      mBox->pTail->pPrevious = tmpMsg;
      mBox->nMessages = mBox->nMessages + 1;
      
    }
    
    LoadContext();
    
  }
  
  return OK;
  
}
exception  receive_no_wait(mailbox *mBox, void *Data){
  
  volatile int first = 1;
  
  //isr_off();
  
  SaveContext();
  
  if(first){
    
    first = 0;
    
    if(no_messages(mBox) != 0 && mBox->pHead->pNext->Status == SENDER){
      
      memcpy(Data, mBox->pHead->pNext->pData,mBox->nDataSize);
      
      if(mBox->nBlockedMsg != 0){
        
        //Detta är ett blockeringsmeddelande
        //Ta bort association mellan tasken och meddelandet
        mBox->pHead->pNext->pBlock->pMessage = NULL;
        
        mBox->pHead->pNext->pBlock->pPrevious->pNext = mBox->pHead->pNext->pBlock->pNext;
        mBox->pHead->pNext->pBlock->pNext->pPrevious = mBox->pHead->pNext->pBlock->pPrevious;
        
        insert(readylist,mBox->pHead->pNext->pBlock,1);
        
        //Uppdatera running pekare
        Running = readylist->pHead->pNext->pTask;
        removeFirstMsg(mBox);
        
      }
      else{
        
        //Vanligt meddelande
        removeFirstMsg(mBox);
        
      }
      
    }
    
    else return FAIL;
    
    LoadContext();
    
  }
  
  return OK;
  
}
void removeFirstMsg(mailbox *mBox){
  
  if(mBox->nBlockedMsg != 0){
    
    //Postlådan innehåller blockerande meddelanden
    msg *tmpMsg = mBox->pHead->pNext;
    
    //Ta bort association med postlådan
    mBox->pHead->pNext->pNext->pPrevious = mBox->pHead;
    mBox->pHead->pNext = mBox->pHead->pNext->pNext;
    
    //Frigör minnespositionen
    freeMem(tmpMsg);
    (mBox->nBlockedMsg)--;
    
  }
  else{
    
    //Postlådan innehåller bara vanliga meddelanden
    msg *tmpMsg = mBox->pHead->pNext;
    
    //Ta bort association med postlådan
    mBox->pHead->pNext->pNext->pPrevious = mBox->pHead;
    mBox->pHead->pNext = mBox->pHead->pNext->pNext;
    
    //Frigör minnespositionen
    //Eftersom vi har ett vanligt meddelande, så använder vi en extra minnesplats till data
    freeMem(tmpMsg->pData);
    freeMem(tmpMsg);
    (mBox->nMessages)--;
    
  }
  
}
int no_messages(mailbox * mailBox){
  
  return mailBox->nMessages + mailBox->nBlockedMsg;
  
}
//-------------------------------