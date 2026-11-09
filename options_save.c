// Milosoiu Andrei
// Email: andrei18.milosoiu@gmail.com

#include "options_save.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SAVE_MAGIC "2048SAVE1"
#define PATH_MAX_LEN 512

// Writes into path the location of the save file, following the XDG convention
// ($XDG_DATA_HOME/play2048/save, or ~/.local/share/play2048/save).
// If create_dir is non-zero the directories leading to the file are created too.
// Returns 0 on success, 1 on failure.
int get_save_path(char *path, const size_t len, const int create_dir) {
	const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
	char dir[PATH_MAX_LEN];
	int n;

	if (xdg != NULL && xdg[0] == '/') {
		if (create_dir && mkdir(xdg, 0755) != 0 && errno != EEXIST) return 1;

		n = snprintf(dir, sizeof(dir), "%s/play2048", xdg);
	} else if (home != NULL && home[0] != '\0') {
		n = snprintf(dir, sizeof(dir), "%s/.local", home);
		if (n < 0 || (size_t)n >= sizeof(dir)) return 1;
		if (create_dir && mkdir(dir, 0755) != 0 && errno != EEXIST) return 1;

		n = snprintf(dir, sizeof(dir), "%s/.local/share", home);
		if (n < 0 || (size_t)n >= sizeof(dir)) return 1;
		if (create_dir && mkdir(dir, 0755) != 0 && errno != EEXIST) return 1;

		n = snprintf(dir, sizeof(dir), "%s/.local/share/play2048", home);
	} else
		return 1;

	if (n < 0 || (size_t)n >= sizeof(dir)) return 1;
	if (create_dir && mkdir(dir, 0755) != 0 && errno != EEXIST) return 1;

	n = snprintf(path, len, "%s/save", dir);
	if (n < 0 || (size_t)n >= len) return 1;

	return 0;
}

// Saves the current game state (including the state needed for undo) to the save file.
// The data is written to a temporary file first and then renamed, so an interrupted save
// can never corrupt an existing one.
// Returns 0 on success, 1 on failure.
int save_game(const int num, const unsigned long win_num, const int board[NMAX][NMAX],
			  const int prev_board[NMAX][NMAX], const int scr, const int prev_scr,
			  const int move_cnt, const int maxn) {
	char path[PATH_MAX_LEN], tmp_path[PATH_MAX_LEN + 8];

	if (get_save_path(path, sizeof(path), 1) != 0) return 1;
	snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

	FILE *f = fopen(tmp_path, "w");
	if (f == NULL) return 1;

	fprintf(f, "%s\n%d %lu %d %d %d %d\n", SAVE_MAGIC, num, win_num, scr, prev_scr, move_cnt, maxn);
	for (int i = 0; i < num; ++i) {
		for (int j = 0; j < num; ++j) fprintf(f, "%d ", board[i][j]);
		fprintf(f, "\n");
	}
	for (int i = 0; i < num; ++i) {
		for (int j = 0; j < num; ++j) fprintf(f, "%d ", prev_board[i][j]);
		fprintf(f, "\n");
	}

	if (fclose(f) != 0 || rename(tmp_path, path) != 0) {
		remove(tmp_path);
		return 1;
	}

	return 0;
}

// Loads a game state from the save file. The output parameters are only modified on success.
// Returns 0 on success, 1 if there is no save file, -1 if the save file is invalid.
int load_game(int *num, unsigned long *win_num, int board[NMAX][NMAX], int prev_board[NMAX][NMAX],
			  int *scr, int *prev_scr, int *move_cnt, int *maxn) {
	char path[PATH_MAX_LEN], magic[32];

	if (get_save_path(path, sizeof(path), 0) != 0) return 1;

	FILE *f = fopen(path, "r");
	if (f == NULL) return 1;

	int n, s, ps, mc, mx;
	unsigned long wn;
	int b[NMAX][NMAX] = {0}, pb[NMAX][NMAX] = {0};
	int ok = 0;

	if (fscanf(f, "%31s", magic) == 1 && strcmp(magic, SAVE_MAGIC) == 0 &&
		fscanf(f, "%d %lu %d %d %d %d", &n, &wn, &s, &ps, &mc, &mx) == 6 && n >= 4 && n <= NMAX &&
		is_valid_win_num(wn) && s >= 0 && ps >= 0 && mc >= 0 && mx >= 0) {
		ok = 1;

		for (int k = 0; k < 2 && ok; ++k) {
			for (int i = 0; i < n && ok; ++i) {
				for (int j = 0; j < n && ok; ++j) {
					int *cell = k == 0 ? &b[i][j] : &pb[i][j];
					if (fscanf(f, "%d", cell) != 1 || *cell < 0) ok = 0;
				}
			}
		}
	}

	fclose(f);
	if (!ok) return -1;

	*num = n;
	*win_num = wn;
	*scr = s;
	*prev_scr = ps;
	*move_cnt = mc;
	*maxn = mx;
	copy_matrix(n, b, board);
	copy_matrix(n, pb, prev_board);

	return 0;
}

// Deletes the save file, if there is one.
void delete_save() {
	char path[PATH_MAX_LEN];

	if (get_save_path(path, sizeof(path), 0) == 0) remove(path);
}

// Prints the usage of the program to the given stream.
void print_usage(FILE *out) {
	fprintf(out,
			"Usage: play2048 [OPTION]\n"
			"\n"
			"Options:\n"
			"  (none)         Start a new game\n"
			"  -r, --resume   Resume the saved game (starts a new game if none is saved)\n"
			"  -h, --help     Show this help message and exit\n");
}

// Parses the command line arguments.
// Sets *resume to 1 if the saved game should be resumed (-r, --resume).
// Returns OPT_RUN if the program should continue, OPT_EXIT_OK if it should exit successfully
// (e.g. after printing the help) and OPT_EXIT_ERR if the arguments are invalid.
int parse_options(const int argc, char *argv[], int *resume) {
	*resume = 0;

	if (argc > 2) {
		fprintf(stderr, "Too many arguments.\n");
		print_usage(stderr);
		return OPT_EXIT_ERR;
	}

	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			print_usage(stdout);
			return OPT_EXIT_OK;
		} else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--resume") == 0) {
			*resume = 1;
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[1]);
			print_usage(stderr);
			return OPT_EXIT_ERR;
		}
	}

	return OPT_RUN;
}
