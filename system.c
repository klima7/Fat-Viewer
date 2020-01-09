#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disc.h"
#include "system.h"
#include "fat.h"

// Funkcja otwierająca katalog
DIR *opendir(const char *path)
{
    // Sprawdzenie czy katalog istnieje - dla każdego katalogu innego od "/" musi istnieć wpis w jakimś innym katalogu
	STAT info;
	int res = stat(path, &info);
	if(strcmp(path, "/") != 0 && (res || !info.directory)) return NULL;

	uint16_t first_cluster = res ? 0 : info.first_cluster;

	// Zaalokowanie uchwytu do katalogu
	DIR *dir = (DIR *)malloc(sizeof(DIR));
	if(dir == NULL) return NULL;

	// Inicjowanie uchwytu do katalogu
	dir->start = first_cluster;
	dir->pos = 0;

	return dir;
}

// Funkcja zamykająca katalog do odczytu
void closedir(DIR *dir)
{
	free(dir);
}

// Funkcja odczytująca kolejny wpis z katalogu
int readdir(DIR *dir, DENTRY *entry)
{
    if(dir == NULL) return 1;

    while(1)
    {
        struct fat_directory_entry_t fat_entry;
        int res = fat_get_entry_by_pos(dir->start, dir->pos, &fat_entry);
        if(res) return 1;

        if(fat_entry.name[0] == 0) return 1;

        if(fat_is_entry_visible(&fat_entry))
        {
            int res = fat_get_long_filename(dir->start, dir->pos, entry->filename);
            if(res) fat_join_filename(fat_entry.name, fat_entry.ext, entry->filename);
            dir->pos++;
            return 0;
        }

        dir->pos++;
    }
}

// Funkcja pobierająca metainformacje o pliku
int stat(const char *path, STAT *stat)
{
	if(path == NULL || stat == NULL) return 1;

    FENTRY fat_entry;
	int res = fat_get_entry_by_path(path, &fat_entry);
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
	
	stat->clusters_count = fat_get_chain_length(fat_entry.file_start);
    stat->first_cluster = fat_entry.file_start;
	
	return 0;
}

// Otwiera plik do czytania lub zapisu
MYFILE *open(const char *path, const char *mode)
{
    // Sprawdzenie czy plik istnieje i nie jest katalogiem
	STAT info;
	int res = stat(path, &info);
	if(res || info.directory) return NULL;

	// Tworzenie uchwytu do pliku
	MYFILE *file = (MYFILE *)malloc(sizeof(MYFILE));
	if(file == NULL) return NULL;

	file->path = strdup(path);
	if(file->path == NULL) return NULL;
	file->pos = 0;

	// Zwrócenie uchwytu do pliku
	return file;
}

// Funkcja zamykająca plik
void close(MYFILE *file)
{
    free(file->path);
}

// Czytanie danych z pliku
int read(void *buffer, uint32_t size, MYFILE *file)
{
    if(buffer==NULL || file==NULL) return 0;

    struct fat_directory_entry_t entry;
    int res = fat_get_entry_by_path(file->path, &entry);
    if(res) return 0;

    // Przerwanie jeśli odczytaliśmy cały plik
    if(file->pos >= entry.file_size) return MY_EOF;

    // Zmienjszenie liczby bajtów do odczytania jeśli byśmy wyszli poza plik
    uint32_t bytes_to_end = entry.file_size - file->pos;
    if(size > bytes_to_end) size = bytes_to_end;

    // Odczytanie danych i zwiększenie wskaźnika
    int read_count = fat_read_file(buffer, entry.file_start, file->pos, size);
    file->pos += read_count;

    // Zwrócenie liczby odczytanych bajtów
	return read_count;
}

uint16_t get_next_cluster(uint16_t cluster)
{
    int err = 0;
    return fat_get_next_cluster(cluster);
}
