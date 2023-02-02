#include "types.h"
#include "pmm.h"
#include "pmmanager.h"
#include "list.h"

extern FreeList FreeArea;     // 空闲区域链表

PMManager_t ff_PMManager;
PMManager_t* get_FF_PMManager();

void ff_free_area_init();
void ff_memmap_init(PageBlock_t* base, size_t n);
PageBlock_t* ff_alloc_pages(size_t n);
void ff_free_pages(PageBlock_t* base, size_t n);

PMManager_t* get_FF_PMManager() {
    ff_PMManager.alloc_pages = ff_alloc_pages;
    ff_PMManager.free_area_init = ff_free_area_init;
    ff_PMManager.free_pages = ff_free_pages;
    ff_PMManager.memmap_init = ff_memmap_init;
    return &ff_PMManager;
}


void ff_free_area_init() {
    initList(&FreeArea.ptr);
    FreeArea.free_page_count = 0;
}
void ff_memmap_init(PageBlock_t* base, size_t n) {
    assert(n > 0, "n is must greater than 0!");
    PageBlock_t* p = base;
    for (;p != base + n;p++) {
        if (p->flags != 0) {
            p->flags = 0;
            p->property = 0;
            p->ref = 0;
        }
    }
    base->property = n;
    SetBitOne(&(base->flags), PGP_property);
    FreeArea.free_page_count += n;
    listAdd(&(FreeArea.ptr), &(base->ptr));
}
PageBlock_t* ff_alloc_pages(size_t n) {
    assert(n > 0, "alloc:n is must greater than 0!");
    if (n > FreeArea.free_page_count)
        return NULL;

    PageBlock_t* page = NULL;
    PageBlock_t* p = NULL;
    list_ptr_t* lp = &(FreeArea.ptr);
    //寻找第一个匹配的空闲块
    while ((lp = listGetNext(lp)) != &(FreeArea.ptr)) {
        p = lp2page(lp, ptr);
        if (p->property >= n) {
            page = p;
            break;
        }
    }
    //重新组织空闲块
    if (page != NULL) {
        if (page->property > n) {
            PageBlock_t* p = page + n;
            p->property = page->property - n;
            SetBitOne(&(p->flags), PGP_property);
            listAdd(&(page->ptr), &(p->ptr));
        }
        listDel(&(page->ptr));
        FreeArea.free_page_count -= n;
        SetBitZero(&(page->flags), PGP_property);
        page->property = n;
    }
    return page;
}
void ff_free_pages(PageBlock_t* base, size_t n) {
    assert(n > 0, "feee:n is must greater than 0!");
    PageBlock_t* p = base;
    //将要释放的内存块设为未使用状态
    for (;p != base + n;p++) {
        if (GetBit(p->flags, PGP_reserved) != 0 || GetBit(p->flags, PGP_property) != 0)
            panic("free page's flags error!");
        p->flags = 0;
        p->ref = 0;
    }
    base->property = n;
    SetBitOne(&(base->flags), PGP_property);

    list_ptr_t* lp = listGetNext(&(FreeArea.ptr));
    while (lp != &(FreeArea.ptr)) {
        //p指向第一个Pages块
        p = lp2page(lp, ptr);

        //要free的块位于下一个块为首部，向下合并
        if (base + base->property == p) {
            base->property += p->property;
            SetBitZero(&(p->flags), PGP_property);
            listDel(&(p->ptr));
        }
        //要free的块位于上一个块尾部，向上合并
        else if (p + p->property == base) {
            p->property += base->property;
            SetBitZero(&(base->flags), PGP_property);
            base = p;
            listDel(&(p->ptr));
        }
        lp = listGetNext(lp);
    }
    FreeArea.free_page_count += n;
    // lp = listGetNext(&(FreeArea.ptr));
    // while (lp != &(FreeArea.ptr)) {
    //     p = lp2page(lp, ptr);
    //     if (base + base->property <= p) {
    //         assert(base + base->property != p, "base + base->property != p");
    //         break;
    //     }
    //     lp = listGetNext(lp);
    // }
    listAdd(&(FreeArea.ptr), &(base->ptr));

}