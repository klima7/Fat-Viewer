#include "disc.h"

// Wirtualnie włożony dysk
static FILE *disc = NULL;

// Włożenie wysku
void disc_set(FILE *d)
{
	disc = d;
}

// Odczyt sektora z dysku
size_t disc_readblock(void* buffer, uint32_t first_block, size_t block_count)
{
	if(disc == NULL || buffer == NULL || block_count == 0) return 0;
		
	int res = fseek(disc, first_block * SECTOR_SIZE, SEEK_SET);
	if(res != 0)return 0;
		
	size_t count = fread(buffer, SECTOR_SIZE, block_count, disc);
	return count;
}

// Zapis sektora na dysk
size_t disc_writeblock(void* buffer, uint32_t first_block, size_t block_count)
{
	if(disc == NULL || buffer == NULL || block_count == 0) return 0;
		
	int res = fseek(disc, first_block * SECTOR_SIZE, SEEK_SET);
	if(res != 0) return 0;
		
	size_t count = fwrite(buffer, SECTOR_SIZE, block_count, disc);
	return count;
}