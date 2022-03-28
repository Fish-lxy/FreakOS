#ifndef STRING_H
#define STRING_H

#include "types.h"

static inline int strlen(const char* src) {
    int length = 0;
    while (*src++)
        ++length;
    return length;
}
static inline void* memcpy(uint8_t* dest, const uint8_t* src, uint32_t len) {
    for (;len != 0;len--) {
        *dest++ = *src++;
    }
    return dest;
}
static inline void* memset(void* dest, uint8_t val, uint32_t len) {
    uint8_t* dst = (uint8_t*) dest;
    for (; len != 0; len--) {
        *dst++ = val;
    }
    return dest;
}
static int memcmp(const void* buffer1, const void* buffer2, int count){
    if (!count)
        return(0);
    while (--count && *(char*) buffer1 == *(char*) buffer2)
    {
        buffer1 = (char*) buffer1 + 1;
        buffer2 = (char*) buffer2 + 1;
    }
    return(*((unsigned char*) buffer1) - *((unsigned char*) buffer2));
}
static inline void bzero(void* dest, uint32_t len) {
    memset(dest, 0, len);
}
static inline int strcmp(const char* str1, const char* str2) {
    int ret;
    size_t i;
    for (i = 0; str1[i] != '\0' || str2[i] != '\0'; i++) {
        ret = str1[i] - str2[i];
        if (ret == 0)
            continue;
        return ret;
    }

    return ret;
}
static inline char* strcpy(char* dest, const char* src) {
    if (src == NULL || dest == NULL)
        return dest;

    size_t i;
    for (i = 0; src[i] != '\0'; i++)
        dest[i] = src[i];

    dest[i] = '\0';

    return dest;
}
static inline char* strcat(char* dest, const char* src) {
    if (src == NULL || dest == NULL)
        return dest;

    size_t dst_len = strlen(dest);
    size_t i;

    for (i = 0; src[i] != '\0'; i++) {
        dest[dst_len + i] = src[i];
    }

    dest[dst_len + i] = '\0';

    return dest;
}



#endif