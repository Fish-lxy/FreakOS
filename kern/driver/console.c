#include "console.h"
#include "cpu.h"
#include "mm.h"
#include "types.h"

// 屏幕被划分为25行，每行80个字符，共2000个字符
// 0xb8000~0xbffff的内存地址空间被映射到文本模式的显存
// 从0xb8000开始，每2个字节表示屏幕上显示的一个字符
// 例如 0000 0111 0010 0000,0x720,0x7为黑底白字 0x20为空格

static uint16_t *VideoAddr = (uint16_t *)(0xB8000 + KERNEL_BASE); //
static uint8_t CursorX = 0;
static uint8_t CursorY = 0;

void consoleClear() {
    uint8_t high_byte = (TC_black << 4) | (TC_white & 0x0F);
    uint16_t blank = ' ' | (high_byte << 8);

    int i;
    for (i = 0; i < 80 * 25; i++) {
        VideoAddr[i] = (uint16_t)blank;
    }

    CursorX = 0;
    CursorY = 0;
    moveCursor();
}
void consoleScroll() {
    uint8_t high_byte = (TC_black << 4) | (TC_white & 0x0F);
    uint16_t blank = ' ' | (high_byte << 8);

    if (CursorY >= 25) {
        int i;
        for (i = 0; i < 24 * 80; i++) {
            VideoAddr[i] = VideoAddr[i + 80];
        }
        for (i = 24 * 80; i < 25 * 80; i++) {
            VideoAddr[i] = blank;
        }
        CursorY = 24;
    }
}
void moveCursor() {
    // 屏幕是 80 字节宽
    uint16_t cursorLocation = CursorY * 80 + CursorX;

    // 两个内部寄存器的编号为14与15，分别表示光标位置
    // 的高8位与低8位。

    outb(0x3D4, 14);                  // 告诉 VGA 设置光标的高字节
    outb(0x3D5, cursorLocation >> 8); // 发送高 8 位
    outb(0x3D4, 15);                  // 告诉 VGA 设置光标的低字节
    outb(0x3D5, cursorLocation);      // 发送低 8 位
}
void consolePutcColor(char c, TEXT_color_t back, TEXT_color_t fore) {
    uint16_t color = ((back << 4) | (fore & 0x0f)) << 8;
    if (c == 0x08 && CursorX) {
        CursorX--;
    } else if (c == 0x09) {
        CursorX = (CursorX + 8) & ~(8 - 1);
    } else if (c == '\r') {
        CursorX = 0;
    } else if (c == '\n') {
        CursorX = 0;
        CursorY++;
    } else if (c >= ' ') {
        VideoAddr[CursorY * 80 + CursorX] = c | color;
        CursorX++;
    }
    // 每 80 个字符一行，满80就换行了
    if (CursorX >= 80) {
        CursorX = 0;
        CursorY++;
    }
    // 如果需要的话滚动屏幕显示
    consoleScroll();
    // 移动硬件的输入光标
    moveCursor();
}
void consolePutc(char c) { consolePutcColor(c, TC_black, TC_white); }
void consoleWriteStr(char *str) {
    while (*str) {
        consolePutcColor(*str, TC_black, TC_white);
        str++;
    }
}
void consoleWriteStrColor(char *str, TEXT_color_t back, TEXT_color_t fore) {
    while (*str) {
        consolePutcColor(*str, back, fore);
        str++;
    }
}
void consoleWriteHex(uint32_t n, TEXT_color_t back, TEXT_color_t fore);
void consoleWriteDec(uint32_t n, TEXT_color_t back, TEXT_color_t fore) {
    char result[32] = {'\0'};
    dec2str(n, result, 10);
    consoleWriteStrColor(result, back, fore);
}
char *dec2str(int32_t num, char *str, int radix) {
    /*索引表*/
    static char index[] = "0123456789ABCDEF";
    unsigned unum; /*中间变量*/
    int i = 0, j, k;
    /*确定unum的值*/
    if (radix == 10 && num < 0) { /*十进制负数*/
        unum = (unsigned)-num;
        str[i++] = '-';
    } else
        unum = (unsigned)num; /*其他情况*/
    /*转换*/
    do {
        str[i++] = index[unum % (unsigned)radix];
        unum /= radix;
    } while (unum);
    str[i] = '\0';
    /*逆序*/
    if (str[0] == '-')
        k = 1; /*十进制负数*/
    else
        k = 0;

    for (j = k; j <= (i - 1) / 2; j++) {
        char temp;
        temp = str[j];
        str[j] = str[i - 1 + k - j];
        str[i - 1 + k - j] = temp;
    }
    return str;
}