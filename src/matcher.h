#ifndef MATCHER_H
#define MATCHER_H

#include <stdbool.h>

#include "string_list.h"

bool is_excluded(const struct StringList *excludes, const char *relative_path);

#endif
