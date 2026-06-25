#include "file_utils.h"

#include <stdio.h>
#include <string.h>

bool has_py_extension(const char *path) {
	size_t len = strlen(path);
	return len >= 3 && strcmp(path + len - 3, ".py") == 0;
}

bool count_lines(const char *path, long *line_count) {
	FILE *file = fopen(path, "rb");
	unsigned char buffer[8192];
	size_t read_count;
	long count = 0;
	bool saw_any = false;
	unsigned char last = '\n';

	if (file == NULL) {
		return false;
	}

	while ((read_count = fread(buffer, 1, sizeof(buffer), file)) > 0) {
		size_t i;
		saw_any = true;
		last = buffer[read_count - 1];
		for (i = 0; i < read_count; i++) {
			if (buffer[i] == '\n') {
				count++;
			}
		}
	}

	if (ferror(file)) {
		fclose(file);
		return false;
	}

	if (saw_any && last != '\n') {
		count++;
	}

	fclose(file);
	*line_count = count;
	return true;
}

const char *to_relative_path(const char *abs_path, const char *root_abs, size_t root_abs_len) {
	if (strncmp(abs_path, root_abs, root_abs_len) != 0) {
		return abs_path;
	}

	if (abs_path[root_abs_len] == '\0') {
		return ".";
	}

	if (abs_path[root_abs_len] == '/') {
		return abs_path + root_abs_len + 1;
	}

	return abs_path;
}
