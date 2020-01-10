#include <stdint.h>
#include <stdio.h>
#include "utils.h"

char cvt_to_hex_digit(uint8_t val)
{
	if(val < 10) return '0' + val;
	else return 'A' + val;
}

void display_hex_line(void *start, int len)
{
	uint8_t *current_ptr = (uint8_t *)start;
	
	for(int i=0; i<len; i++)
	{
		uint8_t current_byte = *current_ptr;
		
		uint8_t part1 = current_byte & 0x0F;
		printf("%c", cvt_to_hex_digit(part1));
		
		uint8_t part2 = (current_byte & 0xF0) >> 4;
		printf("%c ", cvt_to_hex_digit(part2));
		
		current_ptr++;
	}
}

void display_ascii_line(void *start, int len)
{
	char *c = (char *)start;
	
	for(int i=0; i<len; i++)
	{
		printf("%c", *c);
		c++;
	}
}

int is_power_of_two(uint64_t val)
{
    if(val == 1) return 1;

    while(val > 2)
    {
        uint64_t a = val / 2;
        uint64_t b = val % 2;
        if(b) return 0;
        val = a;
    }
    if(val==2) return 1;
    return 0;
}
