#define _XOPEN_SOURCE 700

#include <errno.h>
#include <ftw.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_utils.h"
#include "matcher.h"
#include "string_list.h"

enum ExitCode {
	EXIT_OK = 0,
	EXIT_VIOLATION = 1,
	EXIT_USAGE = 2,
	EXIT_RUNTIME = 3,
};

struct AppContext {
	const char *root_abs;
	size_t root_abs_len;
	struct StringList suffixes;
	struct StringList excludes;
	long max_lines;
	long files_scanned;
	long files_excluded;
	long violations;
	bool read_error;
};

static struct AppContext g_ctx;

static void print_usage(const char *argv0) {
	fprintf(
		stderr,
		"Usage: %s --suffix SUFFIX [--suffix SUFFIX ...] [--max-lines N] [--exclude PATTERN ...] [PATH]\n"
		"\n"
		"Recursively scan PATH (default: .), checking only files matching the provided suffixes.\n"
		"Exit with status 1 if any file exceeds --max-lines (default: 200).\n"
		"\n"
		"Options:\n"
		"  -s, --suffix SUFFIX   File suffix to scan (repeatable, required)\n"
		"  -m, --max-lines N      Maximum allowed lines per file (default: 200)\n"
		"  -e, --exclude PATTERN  Exclude pattern (repeatable)\n"
		"  -h, --help             Show this help\n",
		argv0
	);
}

static int scan_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	const char *rel_path;
	long lines;
	(void)sb;
	(void)ftwbuf;

	if (typeflag != FTW_F) {
		return 0;
	}

	rel_path = to_relative_path(fpath, g_ctx.root_abs, g_ctx.root_abs_len);

	if (!path_has_suffix(rel_path, &g_ctx.suffixes)) {
		return 0;
	}

	if (is_excluded(&g_ctx.excludes, rel_path)) {
		g_ctx.files_excluded++;
		return 0;
	}

	g_ctx.files_scanned++;

	if (!count_lines(fpath, &lines)) {
		fprintf(stderr, "error: failed to read %s: %s\n", rel_path, strerror(errno));
		g_ctx.read_error = true;
		return 1;
	}

	if (lines > g_ctx.max_lines) {
		fprintf(
			stderr,
			"violation: %s has %ld lines (max: %ld)\n",
			rel_path,
			lines,
			g_ctx.max_lines
		);
		g_ctx.violations++;
	}

	return 0;
}

int main(int argc, char **argv) {
	int opt;
	int option_index = 0;
	long parsed_max;
	char *end = NULL;
	const char *target_path = ".";
	char *root_abs;
	int walk_result;

	static struct option long_options[] = {
		{"suffix", required_argument, NULL, 's'},
		{"max-lines", required_argument, NULL, 'm'},
		{"exclude", required_argument, NULL, 'e'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0},
	};

	memset(&g_ctx, 0, sizeof(g_ctx));
	g_ctx.max_lines = 200;

	while ((opt = getopt_long(argc, argv, "s:m:e:h", long_options, &option_index)) != -1) {
		switch (opt) {
			case 's':
				if (optarg[0] == '\0') {
					fprintf(stderr, "error: --suffix must not be empty\n");
					string_list_free(&g_ctx.suffixes);
					string_list_free(&g_ctx.excludes);
					return EXIT_USAGE;
				}
				if (!string_list_add(&g_ctx.suffixes, optarg)) {
					fprintf(stderr, "error: out of memory while storing --suffix\n");
					string_list_free(&g_ctx.suffixes);
					string_list_free(&g_ctx.excludes);
					return EXIT_RUNTIME;
				}
				break;
			case 'm':
				errno = 0;
				parsed_max = strtol(optarg, &end, 10);
				if (errno != 0 || end == optarg || *end != '\0' || parsed_max <= 0) {
					fprintf(stderr, "error: --max-lines must be a positive integer\n");
					string_list_free(&g_ctx.suffixes);
					string_list_free(&g_ctx.excludes);
					return EXIT_USAGE;
				}
				g_ctx.max_lines = parsed_max;
				break;
			case 'e':
				if (!string_list_add(&g_ctx.excludes, optarg)) {
					fprintf(stderr, "error: out of memory while storing --exclude\n");
					string_list_free(&g_ctx.suffixes);
					string_list_free(&g_ctx.excludes);
					return EXIT_RUNTIME;
				}
				break;
			case 'h':
				print_usage(argv[0]);
				string_list_free(&g_ctx.suffixes);
				string_list_free(&g_ctx.excludes);
				return EXIT_OK;
			default:
				print_usage(argv[0]);
				string_list_free(&g_ctx.suffixes);
				string_list_free(&g_ctx.excludes);
				return EXIT_USAGE;
		}
	}

	if (g_ctx.suffixes.count == 0) {
		fprintf(stderr, "error: at least one --suffix value is required\n");
		print_usage(argv[0]);
		string_list_free(&g_ctx.suffixes);
		string_list_free(&g_ctx.excludes);
		return EXIT_USAGE;
	}

	if (optind < argc) {
		target_path = argv[optind++];
	}
	if (optind < argc) {
		fprintf(stderr, "error: too many positional arguments\n");
		print_usage(argv[0]);
		string_list_free(&g_ctx.suffixes);
		string_list_free(&g_ctx.excludes);
		return EXIT_USAGE;
	}

	root_abs = realpath(target_path, NULL);
	if (root_abs == NULL) {
		fprintf(stderr, "error: unable to resolve path %s: %s\n", target_path, strerror(errno));
		string_list_free(&g_ctx.suffixes);
		string_list_free(&g_ctx.excludes);
		return EXIT_RUNTIME;
	}

	g_ctx.root_abs = root_abs;
	g_ctx.root_abs_len = strlen(root_abs);

	walk_result = nftw(root_abs, scan_callback, 32, FTW_PHYS);
	if (walk_result != 0 && !g_ctx.read_error) {
		free(root_abs);
		string_list_free(&g_ctx.suffixes);
		string_list_free(&g_ctx.excludes);
		return EXIT_RUNTIME;
	}

	fprintf(
		stderr,
		"summary: scanned=%ld excluded=%ld violations=%ld max_lines=%ld\n",
		g_ctx.files_scanned,
		g_ctx.files_excluded,
		g_ctx.violations,
		g_ctx.max_lines
	);

	free(root_abs);
	string_list_free(&g_ctx.suffixes);
	string_list_free(&g_ctx.excludes);

	if (g_ctx.read_error) {
		return EXIT_RUNTIME;
	}
	if (g_ctx.violations > 0) {
		return EXIT_VIOLATION;
	}
	return EXIT_OK;
}
