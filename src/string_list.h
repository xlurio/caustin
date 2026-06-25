#ifndef STRING_LIST_H
#define STRING_LIST_H

#include <stdbool.h>
#include <stddef.h>

struct StringList {
	char **items;
	size_t count;
	size_t capacity;
};

void string_list_free(struct StringList *list);
bool string_list_add(struct StringList *list, const char *value);

#endif
