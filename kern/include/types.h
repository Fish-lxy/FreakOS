#ifndef __TYPES_H
#define __TYPES_H

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef int bool;
typedef unsigned char  uint8_t;
typedef          char  int8_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned int   uint32_t;
typedef          int   int32_t;

typedef uint32_t size_t;



//向下取整
#define ROUNDDOWN(a, n) ({                                          \
            size_t __a = (size_t)(a);                               \
            (typeof(a))(__a - __a % (n));                           \
        })

//向上取整
#define ROUNDUP(a, n) ({                                            \
            size_t __n = (size_t)(n);                               \
            (typeof(a))(ROUNDDOWN((size_t)(a) + __n - 1, __n));     \
        })


//返回结构体 type 中成员 member 的字节偏移
#define offsetof(type, member)                                      \
    ((size_t)(&((type *)0)->member))

// to_struct - 通过结构体成员地址计算结构体首地址
#define to_struct(ptr, type, member)                               \
    ((type *)((char *)(ptr) - offsetof(type, member)))

//获取无符号数 num 中第 pos 位 (从第 0 位开始)
static inline uint32_t GetBit(uint32_t num, uint32_t pos) {
    return (num >> pos) & 1;
}
//将无符号数 num 中第 pos 位置 1 (从第 0 位开始)
static inline void SetBit(uint32_t *num, uint32_t pos){
    (*num) = (*num) | (1 << (pos));  
}
//将无符号数 num 中第 pos 位置 0 (从第 0 位开始)
static inline void ClearBit(uint32_t *num, uint32_t pos){  
    (*num) = (*num) & ~(1 << (pos));  
}


#define E_OK 0
#define E_PNULL 1
#define E_NOT_FILE 2
#define E_NOT_DIR 3
#define E_DENIED 4
#define E_EXIST 5
#define E_NOT_EXIST 6
#define E_DISK_ERR 7
#define E_INTERNAL_ERR 8
#define E_NOT_DEV 9
#define E_OP_NULL 10
#define E_INVAL 11
#define E_NOMEM 12
#define E_KILLED 13
#define E_INVAL_ELF 14
#define E_NO_FREE_TASK 15
#define E_BAD_PROC 16
#define E_UNKNOWN 255



#endif