#include "string_list.h"

#include <stdlib.h>
#include <string.h>

void string_list_free(struct StringList *list) {
	size_t i;
	for (i = 0; i < list->count; i++) {
		free(list->items[i]);
	}
	free(list->items);
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

bool string_list_add(struct StringList *list, const char *value) {
	char *copy;
	char **new_items;
	size_t new_capacity;
	size_t len;

	if (list->count == list->capacity) {
		new_capacity = (list->capacity == 0) ? 8 : list->capacity * 2;
		new_items = realloc(list->items, new_capacity * sizeof(*new_items));
		if (new_items == NULL) {
			return false;
		}
		list->items = new_items;
		list->capacity = new_capacity;
	}

	len = strlen(value) + 1;
	copy = malloc(len);
	if (copy == NULL) {
		return false;
	}
	memcpy(copy, value, len);

	list->items[list->count++] = copy;
	return true;
}
