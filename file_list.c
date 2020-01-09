#include <stdlib.h>
#include <string.h>
#include "file_list.h"

FLIST *file_list_create(void)
{
	FLIST *list = (FLIST *)malloc(sizeof(FLIST));
	if(list == NULL) return NULL;
	list->head = NULL;
	list->tail = NULL;
	return list;
}

void file_list_destroy(FLIST* list)
{
	while(list->head != NULL)
		file_list_remove(list, &list->head->data);
	free(list);
}

MYFILE *file_list_push_back(FLIST *list, MYFILE *file)
{
	FNODE *node = (FNODE *)malloc(sizeof(FNODE));
	if(node == NULL) return NULL;
	
	node->data = *file;
	node->prev = list->tail;
	node->next = NULL;
	
	if(list->head == NULL)
	{
		list->head = node;
		list->tail = node;
	}
	else
	{
		list->tail->next = node;
		list->tail = node;
	}
	
	return &node->data;
}

void file_list_remove(FLIST* list, MYFILE *file)
{
	FNODE *to_remove = (FNODE *)file;
	
	if(to_remove->prev) to_remove->prev->next = to_remove->next;
	if(to_remove->next) to_remove->next->prev = to_remove->prev;
	
	if(to_remove == list->head)
		list->head = list->head->next;
	if(to_remove == list->tail)
		list->tail = list->tail->prev;
		
	free(to_remove);
}

bool file_list_contains(FLIST *list, const char *path)
{
	FNODE *current = list->head;
	while(current != NULL)
	{
		if(strcmp(current->data.path, path) == 0)
			return true;
			
		current = current->next;
	}
	
	return false;
}