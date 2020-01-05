#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "terminal.h"
#include "system.h"
#include "fat.h"

#define MAX_COMMAND_LEN 255
#define MAX_PATH_LEN 	4096

// Aktualny katalog roboczy
static char pwd[MAX_PATH_LEN] = "/";

// Funkcja łączy ścieżki p1 i p2 oraz zapisuje wynik do result
static char *terminal_join_path(char *p1, char *p2, char *result)
{
	if(strlen(p1) + strlen(p2) + 2 >= MAX_PATH_LEN)
		return NULL;
	
	strcpy(result, p1);
	if(strcmp(p1, "/") != 0) strcat(result, "/");
	strcat(result, p2);
	
	return result;
}

// Funkcja oddziela od ścieżki ostatni człon
static char *terminal_truncate_path(char *p1)
{
    char *last = strrchr(p1, '/');
    if(last == NULL) return NULL;
    if(last == p1) strcpy(p1, "/");
    else *last = 0;
    return p1;
}

// Funkcja wyświetlająca czas w czytelny sposób
static void terminal_display_time(struct tm *time)
{
	int day = time->tm_mday;
	int month = time->tm_mon + 1;
	int year = time->tm_year + 1900;
	
	int hour = time->tm_hour + 1;
	int min = time->tm_min;
	int sec = time->tm_sec;
	
	printf("%02d/%02d/%04d  %02d:%02d:%02d", day, month, year, hour, min, sec);
}

// Funkcja zwracająca wskaźnik na napis o numerze nr z sekwencji napisów znajdujących się jeden po drugim
static char *terminal_get_token(char *str, uint32_t nr)
{
	for(uint32_t i=0; i<nr; i++)
	{
		uint32_t len = strlen(str);
		str += len + 1;
		while(*str==0) str++;
	}
	return str;
}

// Polecenie "dir"
static void terminal_command_dir(int argc, char *argv)
{
    // Otworzenie folderu do odczytu
	DIR *dir = opendir(pwd);
	if(dir == NULL)
    {
	    printf("Unable to open directory\n");
	    return;
    }

	// Wczytywanie z folderu wszystkich wpisów
	DENTRY* entry = readdir(dir);
	while(entry != NULL)
	{
	    // Utworzenie pełnej ścieżki do pliku
		char compound[MAX_PATH_LEN];
		if(terminal_join_path(pwd, entry->filename, compound) == NULL) continue;

		// Pobranie metainformacji
		STAT info;
		int res = stat(compound, &info);
		if(res == 0)
		{
			terminal_display_time(&info.modify_time);
			printf("    ");
			if(info.directory) printf("%10s", "<DIR>");
			else printf("%10d", info.size);
			printf("   %s\n", entry->filename);
            free(info.clusters_chain);
		}

		// Pobranie kolejnego wpisu z folderu
		entry = readdir(dir);
	}

	// Zamykanie katalogu
	closedir(dir);
}

// Komenda "cd"
static void terminal_command_cd(int argc, char *argv)
{
    // Sprawdzenie czy podano wymagany argument
	if(argc < 2)
	{
		printf("Correct use: cd [directory name]\n");
		return;
	}

	char new_pwd[MAX_PATH_LEN];
	char *path = terminal_get_token(argv, 1);

	// Stworzenie nowej ścieżki
	strcpy(new_pwd, pwd);
	if(strcmp(pwd, "/") != 0) strcat(new_pwd, "/");
	strcat(new_pwd, path);

	// Otworzenie folderu by sprawdzić czy istnieje
	DIR *new_dir = opendir(new_pwd);
	if(new_dir == NULL)
	{
		printf("Invalid path\n");
		return;
	}
	closedir(new_dir);

	// Zmiana katalogu roboczego
	if(strcmp(path, ".") == 0) return;
	if(strcmp(path, "..") == 0) terminal_truncate_path(pwd);
	else strcpy(pwd, new_pwd);
}

// Komenda "pwd"
static void terminal_command_pwd(int argc, char *argv)
{
	printf("Aktualny katalog: %s\n", pwd);
}

// Komenda "fileinfo"
static void terminal_command_fileinfo(int argc, char *argv)
{
    // Sprawdzenie liczby argumentów
	if(argc < 2)
	{
		printf("Correct use: fileinfo [filename]\n");
		return;
	}	

	// Wydobycie argumentu
	char *filename = terminal_get_token(argv, 1);

	// Stworzenie pełnej ścieżki
	char full_path[MAX_PATH_LEN];
	if(terminal_join_path(pwd, filename, full_path) == NULL)
	{
		printf("Path too long\n");
		return;
	}

	// Pobranie metainformacji i sprawdzenie czy plik istnieje
	STAT info;
	int res = stat(full_path, &info);
	if(res != 0)
	{
		printf("Invalid filename\n");
		return;
	}

    // Zmienne pomocnicze dla wyświetlania
	char a = info.archive ? '+' : '-';
	char r = info.read_only ? '+' : '-';
	char s = info.system ? '+' : '-';
	char h = info.hidden ? '+' : '-';
	char d = info.device ? '+' : '-';
	char v = info.volume ? '+' : '-';

    // Wyświetlenie informacji
	printf("Full path     : %s\n", full_path);
	printf("Attributes    : A:%c R:%c S:%c H:%c D:%c V:%c\n", a, r, s, h, d, v);
	printf("Update time   : ");
	terminal_display_time(&info.modify_time);
	printf("\n");
	printf("Access time   : ");
	terminal_display_time(&info.access_time);
	printf("\n");
	printf("Create time   : ");
	terminal_display_time(&info.create_time);
	printf("\n");
	printf("Size          : %u bytes\n", info.size);
	printf("Clusters count: %u\n", info.clusters_count);
	printf("Clusters      : ");

	// Sytuacja gdy plik nie posiada danych
	if(info.clusters_count == 0)
	    printf("None");

	// Wyświetlenie pierwszego klastra
	if(info.clusters_count > 0)
        printf("[%hu] ", info.clusters_chain[0]);

	// Wyświetlenie reszty klastrów
    for(uint32_t i=1; i<info.clusters_count; i++)
        printf("%hu ", info.clusters_chain[i]);
    printf("\n");

    // Zwolnienie łańcucha klastrów
    free(info.clusters_chain);
}

// Komenda "cat"
static void terminal_command_cat(int argc, char *argv)
{
    // Sprawdzenie liczby argumentów
    if(argc < 2)
    {
        printf("Correct use: cat [filename]\n");
        return;
    }

    // Znalezienie argumentu
    char *filename = terminal_get_token(argv, 1);

    // Stworzenie pełnej ścieżki
    char full_path[MAX_PATH_LEN];
    if(terminal_join_path(pwd, filename, full_path) == NULL)
    {
        printf("Path too long\n");
        return;
    }

    // Pobranie metainformacji i upewnienie się czy plik istnieje
    STAT info;
    int res = stat(full_path, &info);
    if(res != 0)
    {
        printf("Invalid filename\n");
        return;
    }
    free(info.clusters_chain);

    // Sprawdzenie czy plik nie jest katalogiem
    if(info.directory)
    {
        printf("Unable to read data from directory\n");
        return;
    }

    // Sprawdzenie czy plik nie jest pusty
    if(info.size == 0)
    {
        printf("[File is empty]\n");
        return;
    }

    // Otworzenie pliku
    MYFILE *f = open(full_path, "r");
    if(f == NULL)
    {
        printf("Unable to open file\n");
        return;
    }

    // Odczytanie pliku
    char buffer[101];
    int read_res = 0;
    do
    {
        read_res = read(buffer, 100, f);
        if(read_res > 0)
        {
            buffer[read_res] = 0;
            printf("%s", buffer);
        }
    } while(read_res != MY_EOF);

    // Zamknięcie pliku
    close(f);
}

// Komenda "spaceinfo"
static void terminal_command_spaceinfo(int argc, char *argv)
{
    // Pobranie liczby sektorów
    uint32_t free, use, bad, end;
    fat_get_cluster_summary(&free, &use, &bad, &end);

    // Pobranie bootsectora
    struct fat_boot_sector_t bs;
    fat_get_boot_sector(&bs);

    // Wyświetlenie liczby klastrów
    printf("Used clusters : %u\n", use);
    printf("Free clusters : %u\n", free);
    printf("End clusters  : %u\n", end);
    printf("Bad clusters  : %u\n", bad);

    // Wyświetlenie rozmiaru klastra
    printf("Cluster size in sectors : %u\n", bs.bpb.sectors_per_cluster);
    printf("Cluster size in bytes   : %u\n", bs.bpb.sectors_per_cluster * bs.bpb.bytes_per_sector);
}

// Komenda "rootinfo"
static void terminal_command_rootinfo(int argc, char *argv)
{
    // Pobranie liczby wolnych wpisów w katalogu głównym
    int free, used;
    int res = fat_get_root_summary(&free, &used);
    if(res)
    {
        printf("Unable to execute\n");
        return;
    }

    printf("Used entries : %u\n", used);
    printf("Free entries : %u\n", free);
    printf("All entries  : %u\n", used+free);
    printf("Percent:     : %.1f%%\n", 100.0*used/(used+free));
}

// Komenda "get"
static void terminal_command_get(int argc, char *argv)
{
    // Sprawdzenie liczby argumentów
    if(argc < 2)
    {
        printf("Correct use: get [filename]\n");
        return;
    }

    // Znalezienie argumentu
    char *filename = terminal_get_token(argv, 1);

    // Stworzenie pełnej ścieżki
    char full_path[MAX_PATH_LEN];
    if(terminal_join_path(pwd, filename, full_path) == NULL)
    {
        printf("Path too long\n");
        return;
    }

    // Pobranie metainformacji i upewnienie się czy plik istnieje
    STAT info;
    int res = stat(full_path, &info);
    if(res != 0)
    {
        printf("Invalid filename\n");
        return;
    }
    free(info.clusters_chain);

    // Sprawdzenie czy plik nie jest katalogiem
    if(info.directory)
    {
        printf("Unable to read data from directory\n");
        return;
    }

    // Otworzenie pliku
    MYFILE *f = open(full_path, "r");
    if(f == NULL)
    {
        printf("Unable to open file\n");
        return;
    }

    // Utworzenie prawdziwego pliku
    FILE *f2 = fopen(filename, "w");
    if(f2 == NULL)
    {
        close(f);
        printf("Unable to create real file\n");
        return;
    }

    // Odczytanie pliku
    char buffer[100];
    int read_res = 0;
    do
    {
        read_res = read(buffer, 100, f);
        if(read_res > 0)
        {
            int write_res = fwrite(buffer, 1, read_res, f2);
            if(write_res != read_res)
            {
                close(f);
                fclose(f2);
                printf("Error while writing to file\n");
                return;
            }
        }
    } while(read_res != MY_EOF);

    printf("File get successfully\n");

    // Zamknięcie plików
    close(f);
    fclose(f2);
}

// Komenda "cat"
static void terminal_command_zip(int argc, char *argv)
{
    // Sprawdzenie liczby argumentów
    if(argc < 4)
    {
        printf("Correct use: zip [input1] [input2] [output]\n");
        return;
    }

    // Znalezienie argumentów
    char *name1 = terminal_get_token(argv, 1);
    char *name2 = terminal_get_token(argv, 2);
    char *name3 = terminal_get_token(argv, 3);

    // Stworzenie pełnych ścieżek
    char full1[MAX_PATH_LEN];
    if(terminal_join_path(pwd, name1, full1) == NULL)
    {
        printf("Path too long\n");
        return;
    }

    char full2[MAX_PATH_LEN];
    if(terminal_join_path(pwd, name2, full2) == NULL)
    {
        printf("Path too long\n");
        return;
    }

    // Pobranie metainformacji i upewnienie się czy pliki istnieją
    STAT info1;
    int res1 = stat(full1, &info1);
    if(res1 != 0)
    {
        printf("Invalid first filename: %s\n", name1);
        return;
    }
    free(info1.clusters_chain);

    STAT info2;
    int res2 = stat(full2, &info2);
    if(res2 != 0)
    {
        printf("Invalid second filename: %s\n", name2);
        return;
    }
    free(info2.clusters_chain);

    // Sprawdzenie czy pliki nie są katalogami
    if(info1.directory)
    {
        printf("File %s can't be a directory\n", name1);
        return;
    }

    if(info2.directory)
    {
        printf("File %s can't be a directory\n", name2);
        return;
    }

    // Otworzenie plików
    MYFILE *f1 = open(full1, "r");
    if(f1 == NULL)
    {
        printf("Unable to open file %s\n", name1);
        return;
    }

    MYFILE *f2 = open(full2, "r");
    if(f2 == NULL)
    {
        close(f1);
        printf("Unable to open file %s\n", name2);
        return;
    }

    // Utworzenie prawdziwego pliku
    FILE *f3 = fopen(name3, "w");
    if(f3 == NULL)
    {
        close(f1);
        close(f2);
        printf("Unable to create file %s\n", name3);
        return;
    }

    // Naprzemienne zapisywanie linii
    int read_res1 = 0;
    int read_res2 = 0;

    while(1)
    {
        // Zapisywanie linii z pierwszego pliku jeżli nie został odczytany cały
        char c = 0;
        while(c != '\n' && read_res1 != MY_EOF)
        {
            read_res1 = read(&c, 1, f1);
            if(read_res1 == MY_EOF) break;
            int write_res = fwrite(&c, 1, 1, f3);
            if(write_res != 1)
            {
                close(f1);
                close(f2);
                fclose(f3);
                printf("Error while writing to file %s\n", name3);
                return;
            }
        }

        // Zapisywanie linii z drugiego pliku jeżeli nie został odczytany cały
        c = 0;
        while(c != '\n' && read_res2 != MY_EOF)
        {
            read_res2 = read(&c, 1, f2);
            if(read_res2 == MY_EOF) break;
            int write_res = fwrite(&c, 1, 1, f3);
            if(write_res != 1)
            {
                close(f1);
                close(f2);
                fclose(f3);
                printf("Error while writing to file %s\n", name3);
                return;
            }
        }

        // Dane z obu plików zostały już odczytane
        if(read_res1 == MY_EOF && read_res2 == MY_EOF) break;
    }

    printf("Zip ended successfully\n");

    // Zamknięcie plików
    close(f1);
    close(f2);
    fclose(f3);
}

// Komenda "rename"
static void terminal_command_rename(int argc, char *argv)
{
    // Sprawdzenie liczby argumentów
    if (argc < 3) {
        printf("Correct use: raname [old name] [new name]\n");
        return;
    }

    // Znalezienie argumentów
    char *old_name = terminal_get_token(argv, 1);
    char *new_name = terminal_get_token(argv, 2);

    // Stworzenie pełnych ścieżek
    char full[MAX_PATH_LEN];
    if (terminal_join_path(pwd, old_name, full) == NULL) {
        printf("Path too long\n");
        return;
    }

    // TODO

    printf("Renamed\n");
}

// Funkcja pobierająca od użytkownika komendę
static void terminal_read_command(char *command)
{
	printf(">> ");
	fgets(command, MAX_COMMAND_LEN, stdin);
	char *n = strrchr(command, '\n');
	if(n) *n = 0;
	else
	{
		while(getchar() != '\n') continue;
	}
}

// Funkcja dzieląca pobraną od użytkownika komendę na tokeny. Ciąg ujęty w cudzysłowy "..." jest traktowany jako jeden token
static int terminal_tokenize_command(char *command)
{
    int quote_flag = 0;
    int argc = 1;
    char *current = command;

    while(*current != 0)
    {
        if(*current == ' ' && !quote_flag) 
        {
            *current = 0;
            argc++;
        }
        if(*current == '\"') 
        {
            *current = 0;
            quote_flag = !quote_flag;
        }

        current++;
    }
    return argc;
}

// Funkcja wykonuje przekazaną jako argument komendę
static int terminal_execute_command(char *command)
{
	int argc = terminal_tokenize_command(command);
	if(strcmp(command, "dir") == 0) terminal_command_dir(argc, command);
	else if(strcmp(command, "cd") == 0) terminal_command_cd(argc, command);
	else if(strcmp(command, "pwd") == 0) terminal_command_pwd(argc, command);
	else if(strcmp(command, "fileinfo") == 0) terminal_command_fileinfo(argc, command);
	else if(strcmp(command, "cat") == 0) terminal_command_cat(argc, command);
	else if(strcmp(command, "spaceinfo") == 0) terminal_command_spaceinfo(argc, command);
	else if(strcmp(command, "rootinfo") == 0) terminal_command_rootinfo(argc, command);
	else if(strcmp(command, "get") == 0) terminal_command_get(argc, command);
	else if(strcmp(command, "zip") == 0) terminal_command_zip(argc, command);
	else if(strcmp(command, "rename") == 0) terminal_command_rename(argc, command);
	else if(strcmp(command, "exit") == 0) return 1;
	else printf("Invalid command\n");
	
	return 0;
}

// Funkcja uruchamiająca terminal działający do podania komendy exit
void terminal_run(void)
{
	printf("Fatviever - build date %s\n", __DATE__);
	
	char command[MAX_COMMAND_LEN];
	
	while(1)
	{
		terminal_read_command(command);
		int res = terminal_execute_command(command);
		if(res) return;
	}
}