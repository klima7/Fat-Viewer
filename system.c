#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disc.h"
#include "system.h"
#include "file_list.h"
#include "fat.h"

static FLIST *files;

int system_init(void)
{	
	files = file_list_create();
	if(files == NULL) return 1;
	
	fat_init();
	return 0;
}

// Funkcja otwierająca katalog
DIR *opendir(const char *path)
{
    // Sprawdzenie czy katalog istnieje - dla każdego katalogu innego od "/" musi istnieć wpis w jakimś innym katalogu
	STAT info;
	int res = stat(path, &info);
	if(strcmp(path, "/") != 0 && (res || !info.directory)) return NULL;

	// TODO Trzeba zwolnić łańcuch
	uint16_t first_cluster = res ? 0 : info.clusters_chain[0];

	// Zaalokowanie uchwytu do katalogu
	DIR *dir = (DIR *)malloc(sizeof(DIR));
	if(dir == NULL) return NULL;

	// Inicjowanie uchwytu do katalogu
	dir->pos = 0;
	dir->entries_count = fat_get_visible_entries_count(first_cluster);
	dir->entries = (DENTRY *)malloc(sizeof(DENTRY) * dir->entries_count);
	if(dir->entries == NULL)
	{
		free(dir);
		return NULL;
	}

	// Uzupełnianie tablicy wpisów
	uint32_t entries_count = fat_get_entries_count(first_cluster);
	DENTRY *sys_entry = dir->entries;
	for(uint32_t i=0; i<entries_count; i++)
	{
		struct fat_directory_entry_t fat_entry;
		int res = fat_get_entry_POS(first_cluster, i, &fat_entry);
		if(res)
        {
		    free(dir->entries);
		    free(dir);
		    return NULL;
        }
		if(!fat_is_entry_visible(&fat_entry)) continue;
		fat_join_filename((char*)fat_entry.name, (char*)fat_entry.ext, (char*)sys_entry->filename);
		sys_entry++;
	}

	// Zwrócenie uchwytu do katalogu
	return dir;
}

void closedir(DIR *dir)
{
	free(dir->entries);
	free(dir);
}

DENTRY *readdir(DIR *dir)
{
	if(dir->pos >= dir->entries_count)
		return NULL;
		
	dir->pos++;
	
	DENTRY *entry = dir->entries + dir->pos - 1;
	return entry;
}

int stat(const char *path, STAT *stat)
{
	if(path == NULL || stat == NULL) return 1;

    FENTRY fat_entry;
	int res = fat_get_entry(path, &fat_entry);
	if(res != 0) return 1;
	
	stat->directory = fat_entry.attr & FAT_ATTR_SUBDIRECTORY;
	stat->hidden = fat_entry.attr & FAT_ATTR_HIDDEN;
	stat->read_only = fat_entry.attr & FAT_ATTR_READ_ONLY;
	stat->system = fat_entry.attr & FAT_ATTR_SYSTEM;
	stat->volume = fat_entry.attr & FAT_ATTR_VOLUME;
	stat->archive = fat_entry.attr & FAT_ATTR_ARCHIVE;
	stat->device = fat_entry.attr & FAT_ATTR_DEVICE;
	
	stat->create_time = fat_convert_time(fat_entry.create_date, fat_entry.create_time);
	stat->modify_time = fat_convert_time(fat_entry.modify_date, fat_entry.modify_time);
	stat->access_time = fat_convert_time(fat_entry.access_date, 0);
	
	stat->size = fat_entry.file_size;
	
	stat->clusters_count = fat_get_chain_length(fat_entry.file_start, NULL);
    stat->clusters_chain = (uint16_t *)malloc(sizeof(uint16_t) * (stat->clusters_count + 1));
    if(stat->clusters_chain == NULL) return 1;

    uint16_t current_cluster = fat_entry.file_start;
    for(int i=0; i<stat->clusters_count; i++)
    {
        stat->clusters_chain[i] = current_cluster;
        current_cluster = fat_get_next_cluster(current_cluster, NULL);
    }
    stat->clusters_chain[stat->clusters_count] = 0;
	
	return 0;
}

// Otwiera plik do czytania lub zapisu
MYFILE *open(const char *path, const char *mode)
{
    // Sprawdzenie czy plik istnieje i nie jest katalogiem
	STAT info;
	int res = stat(path, &info);
	if(res || info.directory) return NULL;

	// Potrzebujemy tylko pierwszego klastra - zapamiętujemy go a reszte zwalniamy
	uint16_t first_cluster = info.clusters_chain[0];
	free(info.clusters_chain);

	// Jeżeli plik jest już otwarty to przerwij
	if(file_list_contains(files, first_cluster))
		return NULL;

	// Tworzenie uchwytu do pliku
	MYFILE file;
	file.start_cluster = first_cluster;
	file.pos = 0;
	file.size = info.size;
    file.write = strchr(mode, 'w') != NULL;
    file.read = strchr(mode, 'r') != NULL;

	// Dodanie pliku do listy otwartych
	MYFILE *file_on_list = file_list_push_back(files, &file);

	// Zwrócenie uchwytu do pliku
	return file_on_list;
}

// Funkcja zamykająca plik
void close(MYFILE *file)
{
    // Usunięcie pliku z listy otwartych
	file_list_remove(files, file);
}

// Czytanie danych z pliku
int read(void *buffer, uint32_t size, MYFILE *file)
{
    // Przerwanie jeśli odczytaliśmy cały plik
    if(file->pos >= file->size) return MY_EOF;

    // Zmienjszenie liczby bajtów do odczytania jeśli byśmy wyszli poza plik
    uint32_t bytes_to_end = file->size - file->pos;
    if(size > bytes_to_end) size = bytes_to_end;

    // Odczytanie danych i zwiększenie wskaźnika
    int read_count = fat_read_file(buffer, file->start_cluster, file->pos, size);
    file->pos += read_count;

    // Zwrócenie liczby odczytanych bajtów
	return read_count;
}