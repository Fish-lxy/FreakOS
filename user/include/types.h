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
typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

typedef uint32_t size_t;

typedef struct Stat_t {
    uint32_t attr;
    uint32_t size;

} Stat_t;

#endif