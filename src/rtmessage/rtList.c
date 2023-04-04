/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
*/
#include "rtList.h"
#include "rtLog.h"
#include "rtMemory.h"
#include <errno.h>
#include <stdlib.h>
#include <memory.h>

void* rtListReuseData = NULL+1;

struct _rtListItem
{
  void* data;
  rtListItem next;
  rtListItem prev;
};

struct _rtList
{
  rtListItem front;
  rtListItem back;
  rtListItem free;/*list of 'freed' items that we recycle to avoid free/malloc*/
  size_t size;
};

static rtListItem rtList_GetFreeItem(rtList list)
{
  rtListItem item;
  if(list->free)
  {
    item = list->free;
    list->free = list->free->next;
    /*do not NULL item->data as the user may want to reuse it*/
  }
  else
  {
    item = (rtListItem)rt_try_malloc(sizeof(struct _rtListItem));
    if(item)
      item->data = NULL;
  }
  return item;
}

static void rtList_SetFreeItem(rtList list, rtListItem item)
{
  if(list->free)
  {
    list->free->prev = item;
    item->next = list->free;
    item->prev = NULL;
    list->free = item;
  }
  else
  {
    list->free = item;
    list->free->prev = list->free->next = NULL;
  }
}

/*In all functions that take a return 
  pointer (rtList*, rtListItem*, void**),
  the function must return NULL on error.  
  This is so the user can check for NULL
  without the having to check the rtError return code*/

rtError rtList_Create(rtList* plist)
{
  rtList list;
  RT_CHECK_INVALID_ARG(plist);
  *plist = NULL;
  list = (rtList)rt_try_malloc(sizeof(struct _rtList));
  RT_CHECK_NO_MEM(list);
  memset(list, 0, sizeof(struct _rtList));
  *plist = list;
  return RT_OK;
}

rtError rtList_Destroy(rtList list, rtList_Cleanup destroyer)
{
  rtListItem item, next;
  RT_CHECK_INVALID_ARG(list);
  if (list->front)
  {
    item = list->front;
    do
    {
      if (destroyer)
        destroyer(item->data);
      next = item->next;
      free(item);
      item = next;
    }while(item);
  }
  if (list->free)
  {
    item = list->free;
    do
    {
      if (destroyer && item->data)
        destroyer(item->data);
      next = item->next;
      free(item);
      item = next;
    }while(item);
  }
  free(list);
  return RT_OK;
}
rtError rtList_PushFront(rtList list, void* data, rtListItem* pitem)
{
  rtListItem item;
  if (pitem)
    *pitem = NULL;
  RT_CHECK_INVALID_ARG(list);
  item = rtList_GetFreeItem(list);
  RT_CHECK_NO_MEM(item);
  if(data != rtListReuseData)
    item->data = data;
  item->prev = NULL;
  item->next = list->front;
  if(list->front)
    list->front->prev = item;
  list->front = item;
  if(list->back == NULL)
    list->back = item;
  list->size++;
  if(pitem)
    *pitem = item;
  return RT_OK;
}
rtError rtList_PushBack(rtList list, void* data, rtListItem* pitem)
{
  rtListItem item;
  if (pitem)
    *pitem = NULL;
  RT_CHECK_INVALID_ARG(list);
  item = rtList_GetFreeItem(list);
  RT_CHECK_NO_MEM(item);
  if(data != rtListReuseData)
    item->data = data;
  item->next = NULL;
  item->prev = list->back;
  if(list->back)
    list->back->next = item;
  list->back = item;
  if(list->front == NULL)
    list->front = item;
  list->size++;
  if(pitem)
    *pitem = item;
  return RT_OK;
}
rtError rtList_InsertBefore(rtList list, void* data, rtListItem at, rtListItem* pitem)
{
  rtListItem item;
  if (pitem)
    *pitem = NULL;
  RT_CHECK_INVALID_ARG(list);
  RT_CHECK_INVALID_ARG(at);
  item = rtList_GetFreeItem(list);
  RT_CHECK_NO_MEM(item);
  if(data != rtListReuseData)
    item->data = data;
  item->next = at;
  item->prev = at->prev;
  at->prev = item;
  if(at == list->front)
    list->front = item;
  else
    item->prev->next = item;
  list->size++;
  if(pitem)
    *pitem = item;
  return RT_OK;
}
rtError rtList_InsertAfter(rtList list, void* data, rtListItem at, rtListItem* pitem)
{
  rtListItem item;
  if (pitem)
    *pitem = NULL;
  RT_CHECK_INVALID_ARG(list);
  RT_CHECK_INVALID_ARG(at);
  item = rtList_GetFreeItem(list);
  RT_CHECK_NO_MEM(item);
  if(data != rtListReuseData)
    item->data = data;
  item->next = at->next;
  item->prev = at;
  at->next = item;
  if(at == list->back)
    list->back = item;
  else
    item->next->prev = item;
  list->size++;
  if(pitem)
    *pitem = item;
  return RT_OK;
}
rtError rtList_RemoveItem(rtList list, rtListItem item, rtList_Cleanup destroyer)
{
#ifdef RT_LIST_ERROR_CHECKING
  rtListItem tmp = list->front;
  while(tmp && tmp != item)
    tmp = tmp->next;
  if(tmp != item)
    rtLog_Error("rtList_RemoveItem list %p doesn't own item %p", list, item);
#endif
  RT_CHECK_INVALID_ARG(list);
  RT_CHECK_INVALID_ARG(item);
  if(item->prev)
    item->prev->next = item->next;
  if(item->next)
    item->next->prev = item->prev;
  if(list->front == item)
    list->front = item->next;
  if(list->back == item)
    list->back = item->prev;
  if(destroyer && item->data)
  {
    destroyer(item->data);
    item->data = NULL;
  }
  rtList_SetFreeItem(list, item);
  list->size--;
  return RT_OK;
}
rtError rtList_RemoveItemWithData(rtList list, void* data, rtList_Cleanup destroyer)
{
  rtListItem item;
  RT_CHECK_INVALID_ARG(list);
  item = list->front;
  while(item && item->data != data)
    item = item->next;
  if(!item)
  {
#ifdef RT_LIST_ERROR_CHECKING
    rtLog_Error("rtList_RemoveItemWithData list %p doesn't own data %p", list, data);
#endif
    return RT_ERROR_INVALID_ARG;
  }
  return rtList_RemoveItem(list, item, destroyer);
}
rtError rtList_RemoveItemByCompare(rtList list, const void* comparison, rtList_Compare compare, rtList_Cleanup destroyer)
{
  rtListItem item = list->front;
  RT_CHECK_INVALID_ARG(list);
  RT_CHECK_INVALID_ARG(comparison);
  RT_CHECK_INVALID_ARG(compare);
  while(item)
  {
    if (compare(item->data, comparison) == 0)
    {
      rtList_RemoveItem(list, item, destroyer);
      break; /*this assumes the list is a set of unique values ( without duplicated) so we can exit early*/
    }
    item = item->next;
  }
  if(item == NULL)
    return RT_ERROR;
  else
    return RT_OK;
}

rtError rtList_RemoveAllItems(rtList list, rtList_Cleanup destroyer)
{
  rtListItem item, next;
  RT_CHECK_INVALID_ARG(list);
  item = list->front;
  while(item)
  {
    next = item->next;
    if(destroyer && item->data)
    {
      destroyer(item->data);
      item->data = NULL;
    }
    rtList_SetFreeItem(list, item);
    item = next;
  }
  list->size = 0;
  list->front = list->back = NULL;
  return RT_OK;
}

int rtList_HasItem(rtList list, const void* comparison, rtList_Compare compare)
{
  if(!list || !comparison || !compare)
    return 0;
  if(rtList_Find(list, comparison, compare))
    return 1;
  else
    return 0;
}

void* rtList_Find(rtList list, const void* comparison, rtList_Compare compare)
{
  rtListItem item;
  if(!list || !comparison || !compare)
    return NULL;
  item = list->front;
  while(item)
  {
    if (compare(item->data, comparison) == 0)
      return item->data;
    item = item->next;
  }
  return NULL;
}

rtError rtList_GetSize(rtList list, size_t* size)
{
  RT_CHECK_INVALID_ARG(size);
  *size = 0;
  RT_CHECK_INVALID_ARG(list);
  *size = list->size;
  return RT_OK;
}
rtError rtList_GetFront(rtList list, rtListItem* pitem)
{
  RT_CHECK_INVALID_ARG(pitem);
  *pitem = NULL;
  RT_CHECK_INVALID_ARG(list);
  *pitem = list->front;
  return RT_OK;
}
rtError rtList_GetBack(rtList list, rtListItem* pitem)
{
  RT_CHECK_INVALID_ARG(pitem);
  *pitem = NULL;
  RT_CHECK_INVALID_ARG(list);
  *pitem = list->back;
  return RT_OK;
}
rtError rtListItem_GetData(rtListItem item, void** data)
{
  RT_CHECK_INVALID_ARG(data);
  *data = NULL;
  RT_CHECK_INVALID_ARG(item);
  *data = item->data;
  return RT_OK;
}
rtError rtListItem_SetData(rtListItem item, void* data)
{
  RT_CHECK_INVALID_ARG(item);
  item->data = data;
  return RT_OK;
}
rtError rtListItem_GetNext(rtListItem item, rtListItem* pitem)
{
  RT_CHECK_INVALID_ARG(pitem);
  *pitem = NULL;
  RT_CHECK_INVALID_ARG(item);
  *pitem = item->next;
  return RT_OK;
}
rtError rtListItem_GetPrev(rtListItem item, rtListItem* pitem)
{
  RT_CHECK_INVALID_ARG(pitem);
  *pitem = NULL;
  RT_CHECK_INVALID_ARG(item);
  *pitem = item->prev;
  return RT_OK;
}

void 
rtList_Cleanup_Free(void* item)
{
    if(item)
    {
        free(item);
    }
}

int 
rtList_Compare_String(const void* left, const void* right)
{
  if(left)
  {
    if(right)
    {
      return strcmp(left, right);
    }
    else
    {
      return 1;
    }
  }
  else
  {
    return -1;
  }
}

#if 0 /* simple unit test 
         run: gcc -o rtListTest rtList.c && ./rtListTest 
      */
rtError rtErrorFromErrno(int err)
{
  return RT_FAIL;
}

void printListWhile(rtList list)
{
  rtListItem item;
  size_t sz;
  void* data;

  rtList_GetSize(list, &sz);
  printf("list size=%lu\n", sz);

  rtList_GetFront(list, &item);
  while(item)
  {
    rtListItem_GetData(item, &data);
    printf("data=%d\n", (int)(long long int)data);
    rtListItem_GetNext(item, &item);
  }
  printFreeList(list);
}
void printListFor(rtList list)
{
  rtListItem item;
  size_t sz;
  void* data;

  rtList_GetSize(list, &sz);
  printf("list size=%lu\n", sz);

  for(rtList_GetFront(list, &item); 
      item != NULL; 
      rtListItem_GetNext(item, &item))
  {
    rtListItem_GetData(item, &data);
    printf("data=%d\n", (int)(long long int)data);
  }
  printFreeList(list);
}
void printListString(rtList list)
{
  rtListItem item;
  size_t sz;
  void* data;

  rtList_GetSize(list, &sz);
  printf("list size=%lu\n", sz);

  for(rtList_GetFront(list, &item); 
      item != NULL; 
      rtListItem_GetNext(item, &item))
  {
    rtListItem_GetData(item, &data);
    printf("data=%s\n", (char*)data);
  }
  printFreeListString(list);
}
int gUseFor = 0;
void printList(rtList list)
{
  if(gUseFor)
    printListFor(list);
  else
    printListWhile(list);
}

int printFreeList(rtList list)
{
  rtListItem item;
  size_t sz = 0;
  item = list->free;
  while(item)
  {
    sz++;
    item = item->next;
  }
  printf("free list size=%lu\n", sz);  
}
int printFreeListString(rtList list)
{
  rtListItem item;
  size_t sz = 0;
  item = list->free;
  while(item)
  {
    sz++;
    printf("free list data=%s\n", (char*)item->data);
    item = item->next;
  }
  printf("free list size=%lu\n", sz);  
}
void freeString(void* s)
{
  free(s);
}
int main(int argc, char* argv[])
{
  rtList list;
  rtListItem items[6];
  rtListItem item, next;
  void* data;
  char* sdata;
  int i;

  rtListReuseData=(void*)-1;

  for(i = 1; i < argc; ++i)
    if(!strcmp(argv[i], "--for"))
      gUseFor = 1;

  printf("create list with 6 items\n"); 
  rtList_Create(&list);
  rtList_PushFront(list, (void*)2, &items[2]);
  rtList_PushFront(list, (void*)1, &items[1]);
  rtList_PushFront(list, (void*)0, &items[0]);
  rtList_PushBack(list, (void*)3, &items[3]);
  rtList_PushBack(list, (void*)4, &items[4]);
  rtList_PushBack(list, (void*)5, &items[5]);
  printList(list);

  printf("removing item 2\n"); 
  rtList_RemoveItem(list, items[2], NULL);
  printList(list);

  printf("removing item 0\n"); 
  rtList_RemoveItem(list, items[0], NULL);
  printList(list);

  printf("removing item 5\n"); 
  rtList_RemoveItem(list, items[5], NULL);
  printList(list);

  printf("insert 0 before 1\n"); 
  rtList_InsertBefore(list, (void*)0, items[1], &items[0]);
  printList(list);

  printf("insert 2 before 3\n"); 
  rtList_InsertBefore(list, (void*)2, items[3], &items[2]);
  printList(list);

  printf("insert 5 after 4\n"); 
  rtList_InsertAfter(list, (void*)5, items[4], &items[5]);
  printList(list);

  printf("pushfront -1\n"); 
  rtList_PushFront(list, (void*)-1, &item);
  printList(list);

  printf("inserver -2 before -1\n"); 
  rtList_InsertBefore(list, (void*)-2, item, &item);
  printList(list);

  printf("inserver -3 before -2\n"); 
  rtList_InsertBefore(list, (void*)-3, item, &item);
  printList(list);

  printf("remove all odd numbers\n");
  rtList_GetFront(list, &item);
  while(item)
  {
    rtListItem_GetNext(item, &next);
    rtListItem_GetData(item, &data);
    if( (((int)(long long int)data) % 2) != 0 )
      rtList_RemoveItem(list, item, NULL);
    item = next;
  }
  printList(list);

  printf("destroy list\n"); 
  rtList_Destroy(list, NULL);

  /*exercise reusability*/
  printf("testing reusability\n"); 
  rtList_Create(&list);
  sdata = rt_malloc(10); strncpy(sdata, "one", 10);
  rtList_PushBack(list, sdata, &items[0]);
  sdata = rt_malloc(10); strncpy(sdata, "two", 10);
  rtList_PushBack(list, sdata, &items[1]);
  sdata = rt_malloc(10); strncpy(sdata, "three", 10);
  rtList_PushBack(list, sdata, &items[2]);
  printf("list should contain one, two, three. free list empty\n");
  printListString(list);
  rtList_RemoveItem(list, items[0], NULL);
  rtList_RemoveItem(list, items[1], NULL);
  printf("list should contain three. free list one, two\n");
  printListString(list);
  rtList_PushBack(list, rtListReuseData, &items[0]);
  rtList_PushBack(list, rtListReuseData, &items[1]);
  rtList_PushBack(list, rtListReuseData, &items[3]);
  rtList_PushBack(list, rtListReuseData, &items[4]);
  printf("list should contain one, two, three, null, null. free list empty\n");
  printListString(list);
  rtList_RemoveItem(list, items[4], NULL);
  rtList_RemoveItem(list, items[3], freeString);
  rtList_PushBack(list, rtListReuseData, &items[3]);
  rtList_PushBack(list, rtListReuseData, &items[4]);
  printf("list should contain one, two, three, null, null. free list empty\n");
  printListString(list);
  printf("setting all data\n");
  for(i=0; i<5; i++)
  {
    char buff[10];
    void* data;
    printf("getting data for %p\n", items[i]->data);
    rtListItem_GetData(items[i], &data);
    if(data)
    {
      printf("got data: %s\n", (char*)data);
    }
    else
    {
      printf("got data null\n");
      data = rt_malloc(10);
      rtListItem_SetData(items[i], data);
    }
    snprintf(buff, 10, "item %d", i);
    strncpy((char*)data, buff, 10);
  }
  printListString(list);
  rtList_Destroy(list, freeString);

  return 0;
}
#endif
