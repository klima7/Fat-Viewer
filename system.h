#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Wartość sygnalizująca koniec pliku
#define MY_EOF  -1

// Uchwyt do katalogu
struct dir_t
{
    uint16_t start;
	uint32_t pos;
};

// Wpis w katalogu
struct dir_entry_t
{
	char filename[257];
	uint32_t type;
};

// Metainformacje związane z plikiem
struct stat_t
{
	uint32_t size;
	struct tm create_time;
	struct tm modify_time;
	struct tm access_time;
	bool read_only;
	bool hidden;
	bool system;
	bool volume;
	bool directory;
	bool archive;
	bool device;
	uint32_t clusters_count;
	uint32_t first_cluster;
};

// Uchwyt do pliku
struct file_t
{
    char *path;
	uint32_t pos;
};

typedef struct dir_t DIR;
typedef struct dir_entry_t DENTRY;
typedef struct stat_t STAT;
typedef struct file_t MYFILE;

// Prototypy
DIR *opendir(const char *path);
void closedir(DIR *dir);
int readdir(DIR *dir, DENTRY *entry);
int stat(const char *path, STAT *stat);
MYFILE *open(const char *path, const char *mode);
void close(MYFILE *file);
int read(void *buffer, uint32_t size, MYFILE *file);
uint32_t get_next_cluster(uint32_t cluster);

#endif