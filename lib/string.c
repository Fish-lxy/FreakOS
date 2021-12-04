#include "string.h"
#include "types.h"
void memcpy(uint8_t* dest, const uint8_t* src, uint32_t len) {
    for (;len != 0;len--) {
        *dest++ = *src++;
    }
}
void memset(void* dest, uint8_t val, uint32_t len) {
    uint8_t* dst = (uint8_t*) dest;

    for (; len != 0; len--) {
        *dst++ = val;
    }
}
void bzero(void* dest, uint32_t len) {
    memset(dest, 0, len);
}
int strcmp(const char* str1, const char* str2) {
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
char* strcpy(char* dest, const char* src) {
    if (src == NULL || dest == NULL)
        return dest;

    size_t i;
    for (i = 0; src[i] != '\0'; i++)
        dest[i] = src[i];

    dest[i] = '\0';

    return dest;
}
char* strcat(char* dest, const char* src) {
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
int strlen(const char* src) {
    int length = 0;
    while (*src++)
        ++length;
    return length;
}
