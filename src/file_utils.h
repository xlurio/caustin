#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdbool.h>
#include <stddef.h>

bool has_py_extension(const char *path);
bool count_lines(const char *path, long *line_count);
const char *to_relative_path(const char *abs_path, const char *root_abs, size_t root_abs_len);

#endif
