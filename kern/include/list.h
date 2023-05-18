#ifndef __LIST_H
#define __LIST_H

#include "types.h"

//带头结点的双向循环侵入式链表
//一般情况下头节点不使用

typedef struct list_ptr_t list_ptr_t;
typedef 
struct list_ptr_t {
    list_ptr_t* prev;
    list_ptr_t* next;
} list_ptr_t;

static inline void initList(list_ptr_t* elem) __attribute__((always_inline));
static inline void listAdd(list_ptr_t* list, list_ptr_t* new) __attribute__((always_inline));
static inline void listAddTail(list_ptr_t* listelem, list_ptr_t* new) __attribute__((always_inline));
static inline void listAddBefore(list_ptr_t* listelem, list_ptr_t* elem) __attribute__((always_inline));
static inline void listAddAfter(list_ptr_t* listelem, list_ptr_t* elem) __attribute__((always_inline));
static inline void listDel(list_ptr_t* listelem) __attribute__((always_inline));
static inline void listDelInit(list_ptr_t* listelem) __attribute__((always_inline));
static inline uint8_t listIsEmpty(list_ptr_t* list) __attribute__((always_inline));
static inline list_ptr_t* listGetNext(list_ptr_t* listelem) __attribute__((always_inline));
static inline list_ptr_t* listGetPrev(list_ptr_t* listelem) __attribute__((always_inline));

static inline void _list_add(list_ptr_t* elem, list_ptr_t* prev, list_ptr_t* next) __attribute__((always_inline));
static inline void _list_del(list_ptr_t* prev, list_ptr_t* next) __attribute__((always_inline));

static inline void initList(list_ptr_t* elem) {
    elem->prev = elem->next = elem;
}

//头插法，新元素将插入在头节点之后
static inline void listAdd(list_ptr_t* list, list_ptr_t* new) {
    listAddAfter(list, new);
}
//尾插
static inline void listAddTail(list_ptr_t* listelem, list_ptr_t* new) {
    listAddBefore(listelem, new);
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

//链表遍历宏函数，两个参数必须是 list_ptr_t* 类型，遍历会忽略头节点
#define listForEach(i, head) \
 for (i = (head)->next; i != (head); i = i->next)

#endif