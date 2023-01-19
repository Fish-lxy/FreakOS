#ifndef PMMANAGER_H
#define PMMANAGER_H
#include "pmm.h"

typedef struct PMManager_t {
    
    void (*free_area_init)();
    void (*memmap_init)(PageBlock_t* base, size_t n);
    PageBlock_t* (*alloc_pages)(size_t n);
    void (*free_pages)(PageBlock_t* base, size_t n);
}PMManager_t;

PMManager_t* get_FF_PMManager();

#endif