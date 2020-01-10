#include <stdio.h>
#include <locale.h>
#include <string.h>
#include "disc.h"
#include "system.h"
#include "terminal.h"
#include "fat.h"

// Funkcja główna
int main(int argc, char **argv)
{
    // Sprawdzenie liczby przekazanych argumentów
	if(argc < 2)
	{
		printf("Correct use: fatview image\n");
		return 1;
	}

	// Otworzenie pliku dysku i sprawdzenie czy istnieje
	FILE *disc = fopen(argv[1], "rb+");
	if(disc == NULL)
	{
		printf("Disc image doesn't exist\n");
		return 1;
	}

	// Start
	disc_set(disc);
    int res = fat_mount(FAT_12);
    if(res != 0)
    {
        printf("Unable to mount as FAT16\n");
        return 0;
    }
	terminal_run();

	return 0;
}
