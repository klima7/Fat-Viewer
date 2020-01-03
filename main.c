#include <stdio.h>
#include "disc.h"
#include "system.h"
#include "terminal.h"

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

	// Wirtualne włożenie dysku do komputera
	disc_set(disc);

	// Uruchomienie systemu
	system_init();

	// Uruchomienie terminala
	terminal_run();

	return 0;
}
