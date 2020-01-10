#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#define COLOR_RESET		"\x1B[0m"
#define COLOR_BLACK		"\x1B[30m"
#define COLOR_RED		"\x1B[31m"
#define COLOR_GREEN		"\x1B[32m"
#define COLOR_YELLOW	"\x1B[33m"
#define COLOR_BLUE		"\x1B[34m"
#define COLOR_MAGENTA	"\x1B[35m"
#define COLOR_CYAN		"\x1B[36m"
#define COLOR_WHITE		"\x1B[37m"

char cvt_to_hex_digit(uint8_t val);
void display_hex_line(void *start, int len);
void display_ascii_line(void *start, int len);
int is_power_of_two(uint64_t val);

#endif