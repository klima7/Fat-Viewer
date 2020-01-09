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

	char *napis = "Ladaa to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, Ladna to bajeczka niedluga, "
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

	/*
    MYFILE *file = open("/plik2", "w");
    if(file == NULL) printf("Error\n");

    int count = write(napis, strlen(napis)+1, file);
    printf("[Count: %d]\n", count);

    close(file);
    */
	// Uruchomienie terminala
	int res = fat_truncate("/plik1", 9000);
	printf("%d\n", res);
	terminal_run();

	return 0;
}
