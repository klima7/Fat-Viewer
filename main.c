#include <stdio.h>
#include <wchar.h>
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

	// Wirtualne włożenie dysku do komputera
	disc_set(disc);

	// Uruchomienie systemu
	system_init();

	char *napis = "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
                  "Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, \n";

	int res = fat_write_file(napis, 5, 0, strlen(napis)+1);
	printf("Write res: %d\n", res);

	// Uruchomienie terminala
	terminal_run();

	return 0;
}
