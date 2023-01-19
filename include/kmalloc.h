#ifndef KMALLOC_H
#define KMALLOC_H


typedef struct BkEntry_t BkEntry_t;
typedef struct Bucket_t Bucket_t;

typedef struct BkEntry_t {
    BkEntry_t* bke_next;
}BkEntry_t;

typedef struct Bucket_t {
    uint32_t size;// 桶的大小
    uint32_t page;// 桶中内存块对应的页面的虚拟地址，按页面大小对齐.
    Bucket_t* next;//指向下一个桶 (each bucket page memory)
    BkEntry_t* entry;// 当前桶的入口, NULL on full.
}Bucket_t;


void* kmalloc(uint32_t bytes);
void kfree(void* p, uint32_t bytes);

void testKmalloc();

#endif