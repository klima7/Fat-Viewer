#include "disc.h"

static FILE *disc = NULL;

void disc_set(FILE *d)
{
	disc = d;
}

size_t disc_readblock(void* buffer, uint32_t first_block, size_t block_count)
{
	if(disc == NULL || buffer == NULL || block_count == 0)
		return 0;
		
	int res = fseek(disc, first_block * SECTOR_SIZE, SEEK_SET);
	
	if(res != 0)
		return 0;
		
	size_t count = fread(buffer, SECTOR_SIZE, block_count, disc);
	
	return count;
}

size_t disc_writeblock(void* buffer, uint32_t first_block, size_t block_count)
{
	if(disc == NULL || buffer == NULL || block_count == 0)
		return 0;
		
	int res = fseek(disc, first_block * SECTOR_SIZE, SEEK_SET);
	
	if(res != 0)
		return 0;
		
	size_t count = fwrite(buffer, SECTOR_SIZE, block_count, disc);
	
	return count;
}