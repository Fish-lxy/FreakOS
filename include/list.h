#ifndef __LIST_H
#define __LIST_H
//https://github.com/whiteflavour/ucore/blob/master/labcodes/lab2/libs/list.h
#include "types.h"

typedef struct list_ptr_t list_ptr_t;
typedef 
struct list_ptr_t {
    list_ptr_t* prev;
    list_ptr_t* next;
} list_ptr_t;

static inline void initList(list_ptr_t* elem) __attribute__((always_inline));
static inline void listAdd(list_ptr_t* listelem, list_ptr_t* elem);
static inline void listAddBefore(list_ptr_t* listelem, list_ptr_t* elem);
static inline void listAddAfter(list_ptr_t* listelem, list_ptr_t* elem);
static inline void listDel(list_ptr_t* listelem);
static inline void listDelInit(list_ptr_t* listelem);
static inline uint8_t listIsEmpty(list_ptr_t* list);
static inline list_ptr_t* listGetNext(list_ptr_t* listelem);
static inline list_ptr_t* listGetPrev(list_ptr_t* listelem);

static inline void _list_add(list_ptr_t* elem, list_ptr_t* prev, list_ptr_t* next);
static inline void _list_del(list_ptr_t* prev, list_ptr_t* next);

static inline void initList(list_ptr_t* elem) {
    elem->prev = elem->next = elem;
}
static inline void listAdd(list_ptr_t* listelem, list_ptr_t* elem) {
    listAddAfter(listelem, elem);
}
static inline void listAddBefore(list_ptr_t* listelem, list_ptr_t* elem) {
    _list_add(elem, listelem->prev, listelem);
}
static inline void listAddAfter(list_ptr_t* listelem, list_ptr_t* elem) {
    _list_add(elem, listelem, listelem->next);
}
static inline void listDel(list_ptr_t* listelem) {
    _list_del(listelem->prev, listelem->next);
}
static inline void listDelInit(list_ptr_t* listelem) {
    listDel(listelem);
    initList(listelem);
}
static inline uint8_t listIsEmpty(list_ptr_t* list) {
    return list->next == list;
}
static inline list_ptr_t* listGetNext(list_ptr_t* listelem) {
    return listelem->next;
}
static inline list_ptr_t* listGetPrev(list_ptr_t* listelem) {
    return listelem->prev;
}

static inline void _list_add(list_ptr_t* elem, list_ptr_t* prev, list_ptr_t* next) {
    prev->next = next->prev = elem;
    elem->next = next;
    elem->prev = prev;
}
static inline void _list_del(list_ptr_t* prev, list_ptr_t* next) {
    prev->next = next;
    next->prev = prev;
}

#endif