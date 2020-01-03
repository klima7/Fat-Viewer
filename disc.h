#ifndef __DISC_H__
#define __DISC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define SECTOR_SIZE		512

void disc_set(FILE *d);
size_t disc_readblock(void* buffer, uint32_t first_block, size_t block_count);
size_t disc_writeblock(void* buffer, uint32_t first_block, size_t block_count);

#endif