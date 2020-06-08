#ifndef _DLIST_H_
#define _DLIST_H_
#include "kernel.h"

list * create_list();
listobj * create_listobj(int num);
void insert(list * mylist, listobj * pObj, int short);
listobj * extract(listobj * pObj);
void moveToFirst(list *, list *, int short);
void searchWL();
void searchTL();
listobj *create_TCBlistobj(TCB *);

#endif