#include "matcher.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const char *path_basename(const char *path) {
	const char *slash = strrchr(path, '/');
	return slash ? slash + 1 : path;
}

static bool match_char_class(const char **pat_ptr, char c) {
	const char *pat = *pat_ptr;
	bool negated = false;
	bool matched = false;
	bool first = true;

	if (*pat == '!' || *pat == '^') {
		negated = true;
		pat++;
	}

	while (*pat && *pat != ']') {
		char start = *pat;
		char end = *pat;

		if (!first && start == '-' && pat[1] != ']' && pat[1] != '\0') {
			pat++;
			start = pat[-2];
			end = *pat;
		}

		if (c >= start && c <= end) {
			matched = true;
		}

		first = false;
		pat++;
	}

	while (*pat && *pat != ']') {
		pat++;
	}
	if (*pat == ']') {
		pat++;
	}

	*pat_ptr = pat;
	return negated ? !matched : matched;
}

static bool glob_match_impl(const char *pat, const char *text, bool slash_sensitive) {
	if (*pat == '\0') {
		return *text == '\0';
	}

	if (pat[0] == '*' && pat[1] == '*') {
		const char *next = pat + 2;

		while (*next == '*') {
			next++;
		}

		if (*next == '/') {
			const char *scan = text;

			if (glob_match_impl(next + 1, text, slash_sensitive)) {
				return true;
			}

			while (*scan) {
				if (*scan == '/' && glob_match_impl(next + 1, scan + 1, slash_sensitive)) {
					return true;
				}
				scan++;
			}
			return false;
		}

		{
			const char *scan = text;
			do {
				if (glob_match_impl(next, scan, slash_sensitive)) {
					return true;
				}
				if (*scan == '\0') {
					break;
				}
				scan++;
			} while (true);
			return false;
		}
	}

	if (*pat == '*') {
		const char *scan = text;
		do {
			if (glob_match_impl(pat + 1, scan, slash_sensitive)) {
				return true;
			}
			if (*scan == '\0') {
				break;
			}
			if (slash_sensitive && *scan == '/') {
				break;
			}
			scan++;
		} while (true);
		return false;
	}

	if (*pat == '?') {
		if (*text == '\0' || (slash_sensitive && *text == '/')) {
			return false;
		}
		return glob_match_impl(pat + 1, text + 1, slash_sensitive);
	}

	if (*pat == '[') {
		const char *class_pat = pat + 1;
		if (*text == '\0' || (slash_sensitive && *text == '/')) {
			return false;
		}
		if (!match_char_class(&class_pat, *text)) {
			return false;
		}
		return glob_match_impl(class_pat, text + 1, slash_sensitive);
	}

	if (*pat == *text) {
		return glob_match_impl(pat + 1, text + 1, slash_sensitive);
	}

	return false;
}

static bool glob_match_path(const char *pattern, const char *path) {
	return glob_match_impl(pattern, path, true);
}

static bool glob_match_name(const char *pattern, const char *name) {
	return glob_match_impl(pattern, name, false);
}

static bool path_has_matching_component(const char *pattern, const char *path) {
	const char *segment = path;
	const char *cursor = path;

	while (true) {
		if (*cursor == '/' || *cursor == '\0') {
			size_t len = (size_t)(cursor - segment);
			char *component = malloc(len + 1);
			bool matched;

			if (component == NULL) {
				return false;
			}

			memcpy(component, segment, len);
			component[len] = '\0';
			matched = glob_match_name(pattern, component);
			free(component);

			if (matched) {
				return true;
			}
			if (*cursor == '\0') {
				break;
			}
			segment = cursor + 1;
		}
		cursor++;
	}

	return false;
}

bool is_excluded(const struct StringList *excludes, const char *relative_path) {
	size_t i;
	for (i = 0; i < excludes->count; i++) {
		const char *pattern = excludes->items[i];
		if (strchr(pattern, '/') != NULL) {
			if (glob_match_path(pattern, relative_path)) {
				return true;
			}
		} else if (glob_match_name(pattern, path_basename(relative_path)) ||
			path_has_matching_component(pattern, relative_path)) {
			return true;
		}
	}
	return false;
}
