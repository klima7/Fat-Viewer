#ifndef __DIR_LIST_H__
#define __DIR_LIST_H__

#include "system.h"

struct file_list_t
{
    struct file_node_t* head;
	struct file_node_t* tail;
};

struct file_node_t
{
    MYFILE data;
	struct file_node_t* prev;
    struct file_node_t* next;
};

typedef struct file_list_t FLIST;
typedef struct file_node_t FNODE;

FLIST *file_list_create(void);
void file_list_destroy(FLIST* list);
MYFILE *file_list_push_back(FLIST *list, MYFILE *file);
void file_list_remove(FLIST* list, MYFILE *file);
bool file_list_contains(FLIST *list, uint32_t addr);

#endif