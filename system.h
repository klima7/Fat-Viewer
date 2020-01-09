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
	uint32_t pos;
	struct dir_entry_t *entries;
	uint32_t entries_count;
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
	uint16_t *clusters_chain;
};

// Uchwyt do pliku
struct file_t
{
    char *path;
	uint32_t pos;
	bool write;
};

typedef struct dir_t DIR;
typedef struct dir_entry_t DENTRY;
typedef struct stat_t STAT;
typedef struct file_t MYFILE;

// Prototypy
int system_init(void);
DIR *opendir(const char *path);
void closedir(DIR *dir);
DENTRY *readdir(DIR *dir);
int stat(const char *path, STAT *stat);
MYFILE *open(const char *path, const char *mode);
void close(MYFILE *file);
int read(void *buffer, uint32_t size, MYFILE *file);
int write(void *buffer, uint32_t size, MYFILE *file);

#endif