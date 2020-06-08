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
  
  mb->pHead = (msg*)allocateMem(sizeof(msg));           //Allokera minne f�r huvudet
  
  if(mb->pHead == NULL){
    //finns inget minne kvar = null
    freeMem(mb);
    return FAIL;
    
  }
  
  mb->pTail = (msg*)allocateMem(sizeof(msg));           //allokera minne f�r svansen
  
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
    
    //Om mottagaren har ett meddelande som v�ntar
    if(no_messages(mailBox) != 0 && mailBox->pHead->pNext->Status == RECEIVER){
      
      //kopiera s�ndar data till mottagare
      memcpy(mailBox->pHead->pNext->pData,Data,mailBox->nDataSize);
      mailBox->pHead->pNext->pBlock->pMessage = NULL;
      
      
      //flytta till readylist
      listobj *obj = mailBox->pHead->pNext->pBlock;
      obj->pPrevious->pNext = obj->pNext;
      obj->pNext->pPrevious = obj->pPrevious;
      
      insert(readylist,obj,1);
      
      Running = readylist->pHead->pNext->pTask;
      
      //FIFO struktur -> ta bort f�rsta meddelandet i postl�dan
      removeFirstMsg(mailBox);
      
    }
    //Om mottagaren/avs�ndaren inte har ett meddelande som v�ntar
    else{
      
      msg * tmpMsg = (msg *)allocateMem(sizeof(msg));
      if(tmpMsg == NULL) return FAIL;
      
      tmpMsg->pData = Data;
      tmpMsg->Status = SENDER;
      tmpMsg->pBlock = readylist->pHead->pNext;
      
      //associera meddelandet till objektet
      readylist->pHead->pNext->pMessage = tmpMsg;
      
      
      //s�tt in blockerande meddelande i postl�da
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
  //Inte f�rsta g�ngen det k�rs
  else{
    
    if(ticks() >= Running->DeadLine){
      
      msg *tmpMsg = readylist->pHead->pNext->pMessage;
      
      //Ta bort association med lista och frig�r sedan minnespositionen
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
    
    //Om vi har meddelande som v�ntar och vi �r s�ndare
    if(no_messages(mBox) != 0 && mBox->pHead->pNext->Status == SENDER){
      
      //Kopiera s�ndardata till mottagaren
      memcpy(Data, mBox->pHead->pNext->pData, mBox->nDataSize);
      
      //Om meddelandet �r av blockeringstyp
      if(mBox->nBlockedMsg != 0){
        
        //Meddelandet var av blockeringstyp -> meddelandet ska flyttas fr�n waitinglist till readylist
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
      //Meddelandet �r inte av blockeringstyp
      else{
        
        removeFirstMsg(mBox);
        
      }
      
    }
    //Vi har inte meddelande som v�ntar eller vi �r mottagaren
    else{
      
      //Allokera meddelande struktur och s�tt in meddelande i postl�da
      msg * tmpMsg = (msg *) allocateMem(sizeof(msg));
      if(tmpMsg == NULL) return FAIL;
      
      //Initiera meddelande data
      tmpMsg->pData = Data;
      tmpMsg->Status = RECEIVER;
      tmpMsg->pBlock = readylist->pHead->pNext;
      
      //Associera meddelandet till tasken
      readylist->pHead->pNext->pMessage = tmpMsg;
      
      //s�tt in blockerande meddelande i postl�da
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
    
    LoadContext();          //h�r blev det konstigt!
    
  } 
  else{
    
    //Deadline f�r tasken som k�rs har uppn�tts
    if(ticks() >= Running->DeadLine){
      
      msg *tmpMsg = readylist->pHead->pNext->pMessage;
      
      //Ta bort association med lista och frig�r sedan minnespositionen
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
      //Detta �r ett blockeringsmeddelande
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
      
      //Postl�dan �r full, ta bort ett meddelande
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
      
      //S�tt in meddelande i postl�da
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
        
        //Detta �r ett blockeringsmeddelande
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
    
    //Postl�dan inneh�ller blockerande meddelanden
    msg *tmpMsg = mBox->pHead->pNext;
    
    //Ta bort association med postl�dan
    mBox->pHead->pNext->pNext->pPrevious = mBox->pHead;
    mBox->pHead->pNext = mBox->pHead->pNext->pNext;
    
    //Frig�r minnespositionen
    freeMem(tmpMsg);
    (mBox->nBlockedMsg)--;
    
  }
  else{
    
    //Postl�dan inneh�ller bara vanliga meddelanden
    msg *tmpMsg = mBox->pHead->pNext;
    
    //Ta bort association med postl�dan
    mBox->pHead->pNext->pNext->pPrevious = mBox->pHead;
    mBox->pHead->pNext = mBox->pHead->pNext->pNext;
    
    //Frig�r minnespositionen
    //Eftersom vi har ett vanligt meddelande, s� anv�nder vi en extra minnesplats till data
    freeMem(tmpMsg->pData);
    freeMem(tmpMsg);
    (mBox->nMessages)--;
    
  }
  
}
int no_messages(mailbox * mailBox){
  
  return mailBox->nMessages + mailBox->nBlockedMsg;
  
}
//-------------------------------