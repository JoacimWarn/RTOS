#ifndef _INTERPROCESSCOMMUNICATION_H_
#define _INTERPROCESSCOMMUNICATION_H_
#include "kernel.h"

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

#endif