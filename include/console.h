#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

typedef enum TEXT_color {
    TC_black = 0,
    TC_blue = 1,
    TC_green = 2,
    TC_cyan = 3,
    TC_red = 4,
    TC_magenta = 5,
    TC_brown = 6,
    TC_white = 7,
    TC_dark_grey = 8,
    TC_light_blue = 9,
    TC_light_green = 10,
    TC_light_cyan = 11,
    TC_light_red = 12,
    TC_light_magenta = 13,
    TC_yellow = 14,   // light brown
    TC_light_white = 15
}TEXT_color_t;

void consoleClear();
void consoleScroll();
void moveCursor();
void consolePutcColor(char c, TEXT_color_t back, TEXT_color_t fore);
void consoleWrite(char* str);
void consoleWriteColor(char* str, TEXT_color_t back, TEXT_color_t fore);
void consoleWriteHex(uint32_t n, TEXT_color_t back, TEXT_color_t fore);
void consoleWriteDec(uint32_t n, TEXT_color_t back, TEXT_color_t fore);
char* dec2str(int32_t num, char* str, int radix);

#endif