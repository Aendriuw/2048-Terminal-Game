// Milosoiu Andrei
// Email: andrei18.milosoiu@gmail.com

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define NMAX 8
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_UP 3
#define KEY_DOWN 4
#define KEY_R 5
#define KEY_Q 6
#define NO_KEY 0
#define FREE_POS_MAX 128
#define SAVE_MAGIC "2048SAVE1"
#define PATH_MAX_LEN 512

const int vi[4] = {0, 0, -1, 1};
const int vj[4] = {-1, 1, 0, 0};
const unsigned long win_options[] = {2048, 4096, 8192, 16384, 32768, 65536, 131072};
#define WIN_OPTIONS_CNT (sizeof(win_options) / sizeof(win_options[0]))

struct termios orig_termios;

// Disables line buffering so keypresses are registered without waiting for Enter. Saves the
// original settings first so they can be restored later by disable_raw_mode().
void enable_view() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Restores the terminal to its original settings.
// Called before exitting the program to avoid leaving the terminal in this state.
void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Copies matrix every value of matrix src into matrix dest.
void copy_matrix(const int num, const int src[NMAX][NMAX], int dest[NMAX][NMAX]) {
	for (int i = 0; i < num; ++i) {
		for (int j = 0; j < num; ++j) {
			dest[i][j] = src[i][j];
		}
	}
}

// Clears the current screen using ANSI escape codes.
void clear_screen() {
	fprintf(stdout, "\033[2J\033[H");
}

// Reads a keypress from stdin and returns it's associated code.
// Returns NO_KEY if no key is currently pressed.
int read_key() {
	char c = 0;
	if (read(STDIN_FILENO, &c, 1) <= 0) return NO_KEY;

	if (c == 'r' || c == 'R') return KEY_R;
	if (c == 'q' || c == 'Q') return KEY_Q;

	if (c == '\033') {
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) <= 0) return NO_KEY;
		if (read(STDIN_FILENO, &seq[1], 1) <= 0) return NO_KEY;

		if (seq[0] == '[') {
			switch (seq[1]) {
			case 'A':
				return KEY_UP;
			case 'B':
				return KEY_DOWN;
			case 'C':
				return KEY_RIGHT;
			case 'D':
				return KEY_LEFT;
			}
		}
	}

	return NO_KEY;
}

// Returns 1 if the given string is a posistive number, 0 otherwise.
int is_num(const char *str) {
	int len = (int)strlen(str);

	for (int i = 0; i < len; i++)
		if (!isdigit((unsigned char)str[i])) return 0;

	return 1;
}

// Function that requires the user to enter a aux_buffer between 4 and 8.
// The entered value is memorised at the address of the num variable.
int read_grid_num(int *num) {
	char aux_buff[32];
	int aux_num = 0, valid_n = 0, enter_att = 0;

	while (!valid_n) {
		fscanf(stdin, "%s", aux_buff);
		if (is_num(aux_buff)) {
			if (sscanf(aux_buff, "%d", &aux_num) != 1) {
				fprintf(stderr, "sscanf() failed.\n");
				return -1;
			}

			if (aux_num < 4 || aux_num > 8)
				fprintf(stdout, "Please enter a number between 4 and 8!\n");
			else
				valid_n = 1;

			if (enter_att == 3) fprintf(stdout, "JUST ENTER A NUMBER BETWEEN 4 AND 8!\n");
			if (enter_att == 5) fprintf(stdout, "Okay... Please... Enter a number between 4 and 8...\n");
			if (enter_att == 7) {
				fprintf(stdout, "Enough!\n");
				return 1;
			}

			++enter_att;
		} else
			fprintf(stdout, "Please enter a number!\n");
	}

	*num = aux_num;
	return 0;
}

// Returns 1 if the given number is a power of two that appears in win_options, 0 otherwise.
int is_valid_win_num(const unsigned long val) {
	if (val == 0 || (val & (val - 1)) != 0) {
		return 0;
	}

	for (size_t i = 0; i < WIN_OPTIONS_CNT; ++i)
		if (win_options[i] == val) return 1;

	return 0;
}

// Prompts the user for the number needed to win, until a valid one (see is_valid_win_num()) is
// entered. The entered value is memorised at the address of the win_num variable.
// Returns 0 on success, 1 if input ended (EOF) or too many invalid attempts were made.
int read_win_num(unsigned long *win_num) {
	char aux_buff[32];
	unsigned long aux_num;

	fprintf(stdout, "Enter the number you want to reach to win (");
	for (size_t i = 0; i < WIN_OPTIONS_CNT; ++i)
		fprintf(stdout, "%lu%s", win_options[i], i + 1 < WIN_OPTIONS_CNT ? ", " : "):\n");

	for (int att = 0; att < 8; ++att) {
		if (fscanf(stdin, "%31s", aux_buff) != 1) return 1;

		if (!is_num(aux_buff) || strlen(aux_buff) > 9) {
			fprintf(stdout, "Please enter a valid win number!\n");
			continue;
		}

		aux_num = strtoul(aux_buff, NULL, 10);
		if (is_valid_win_num(aux_num)) {
			*win_num = aux_num;
			return 0;
		}

		fprintf(stdout, "The win number must be a power of two from the list above!\n");
	}

	fprintf(stdout, "Enough!\n");
	return 1;
}

// Outputs the current score to stdout.
void print_score(const int scr) {
	fprintf(stdout, "Score: %d\n", scr);
}

// Returns a random digit between 2 and 4.
// 4 has a chance of 1/8 to be returned, while 2 has a chance of 7/8.
int random_digit() {
	int chn = rand() % 8;

	if (chn == 0)
		return 4;
	else
		return 2;
}

// Writes the current board to stdout.
// Where there is an empty cell (the cell has a value of 0), it outputs empty spaces.
void write_board(const int scr, const int num, int board[NMAX][NMAX], int *maxn) {
	fprintf(stdout, "\n");

	for (int i = 0; i < num; ++i) {
		for (int j = 0; j < num; ++j) {
			if (board[i][j] > *maxn) *maxn = board[i][j];

			int val_cpy = board[i][j], dig_cnt = 0;

			while (val_cpy != 0) {
				++dig_cnt;
				val_cpy /= 10;
			}

			if (dig_cnt == 0)
				fprintf(stdout, "[     ] ");
			else {
				if (dig_cnt == 1)
					fprintf(stdout, "[  %d  ] ", board[i][j]);
				else if (dig_cnt == 2) {
					fprintf(stdout, "[  %d ] ", board[i][j]);
				} else if (dig_cnt == 3) {
					fprintf(stdout, "[ %d ] ", board[i][j]);
				} else
					fprintf(stdout, "[ %d] ", board[i][j]);
			}
		}
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");

	print_score(scr);
}

// Builds the prev_board board needed for the undo function and memorises the previous score.
void build_prev_board(const int num, int board[NMAX][NMAX], int prev_board[NMAX][NMAX],
					  const int scr, int *prev_scr) {
	copy_matrix(num, board, prev_board);

	*prev_scr = scr;
}

// Compares the current board with it's previous state (prev_board).
// Returns 1 if any cell differs (a move had any effect), 0 otherwise.
int is_move_valid(const int num, int board[NMAX][NMAX], int prev_board[NMAX][NMAX]) {
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < num; ++j)
			if (board[i][j] != prev_board[i][j]) return 1;

	return 0;
}

// Returns 1 if there are any possible moves left, 0 otherwise.
int is_any_possible_move(const int num, int board[NMAX][NMAX]) {
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < num - 1; ++j)
			if (board[i][j] == board[i][j + 1]) return 1;

	for (int j = 0; j < num; ++j)
		for (int i = 0; i < num - 1; ++i)
			if (board[i][j] == board[i + 1][j]) return 1;

	return 0;
}

// Spawns a new number in a random free cell.
// The free cells' coordinates are memorised inside an array.
void spawn(int n, int board[NMAX][NMAX], int *ok_game) {
	int free_cnt = 0, plc, free_pos[FREE_POS_MAX];

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (board[i][j] == 0) {
				free_pos[2 * free_cnt] = i;
				free_pos[2 * free_cnt + 1] = j;

				++free_cnt;
			}
		}
	}

	if (free_cnt != 0) {
		plc = rand() % free_cnt;
		board[free_pos[2 * plc]][free_pos[2 * plc + 1]] = random_digit();
	} else {
		if (!is_any_possible_move(n, board)) {
			plc = rand() % free_cnt;
			board[free_pos[2 * plc]][free_pos[2 * plc + 1]] = random_digit();
		} else {
			*ok_game = 1;
		}
	}
}

// Slides a number into an empty space in the given direction on the x-axis.
void slide_x(int *j, int *i, int curr_dir, const int num, int board[NMAX][NMAX]) {
	int aux_i = *i;
	int aux_j = *j;

	while (board[aux_i][aux_j] != 0 && board[aux_i + vi[curr_dir]][aux_j + vj[curr_dir]] == 0 &&
		   (aux_j > -curr_dir && aux_j < num - curr_dir)) {
		board[aux_i + vi[curr_dir]][aux_j + vj[curr_dir]] = board[aux_i][aux_j];
		board[aux_i][aux_j] = 0;

		aux_j += vj[curr_dir];
		aux_i += vi[curr_dir];
	}

	*j = aux_j;
	*i = aux_i;
}

// Slides a number into an empty space in the given direction on the y-axis.
void slide_y(int *j, int *i, int curr_dir, const int num, int board[NMAX][NMAX]) {
	int aux_i = *i;
	int aux_j = *j;

	while (board[aux_i][aux_j] != 0 && board[aux_i + vi[curr_dir]][aux_j + vj[curr_dir]] == 0 &&
		   (aux_i > -(curr_dir % 2) && aux_i < num - (curr_dir % 2))) {
		board[aux_i + vi[curr_dir]][aux_j + vj[curr_dir]] = board[aux_i][aux_j];
		board[aux_i][aux_j] = 0;

		aux_j += vj[curr_dir];
		aux_i += vi[curr_dir];
	}

	*j = aux_j;
	*i = aux_i;
}

// Merges a cell with an equal neighbour cell in the given direction (curr_dir), if possible.
// Doubles the neighbour's value, clears the current cell, and adds the
// merged value to the score.
// Updates the current position to the position after the merge.
void slide_merge(int *j, int *i, int curr_dir, int board[NMAX][NMAX], int *scr) {
	int aux_i = *i;
	int aux_j = *j;

	if (board[aux_i][aux_j] != 0 &&
		board[aux_i + vi[curr_dir]][aux_j + vj[curr_dir]] == board[aux_i][aux_j]) {
		*scr += (2 * board[aux_i][aux_j]);

		board[aux_i + vi[curr_dir]][aux_j + vj[curr_dir]] *= 2;
		board[aux_i][aux_j] = 0;

		aux_j += vj[curr_dir];
		aux_i += vi[curr_dir];
	}

	*i = aux_i;
	*j = aux_j;
}

// Processes a move in the 'right' direction for the entire board (row by row).
void move_right(const int num, int board[NMAX][NMAX], int *scr) {
	int jc, okd, okd2;
	for (int i = 0; i < num; ++i) {
		okd = 1;
		okd2 = 1;

		for (int j = num - 2; j >= 0; --j) {
			if (board[i][j] != 0 && board[i][j + 1] == 0) {
				jc = j;
				slide_x(&jc, &i, 1, num, board);

				if (jc < num - 1 && board[i][jc] != 0 && board[i][jc + 1] == board[i][jc] && okd2) {
					slide_merge(&jc, &i, 1, board, scr);
					okd = !okd;
				}

			} else if (board[i][j] != 0 && board[i][j + 1] == board[i][j] && okd2) {
				jc = j;
				slide_merge(&jc, &i, 1, board, scr);

				okd = !okd;
			}

			if (okd == 0) okd2 = !okd2;
		}
	}
}

// Processes a move in the 'left' direction for the entire board (row by row).
void move_left(const int num, int board[NMAX][NMAX], int *scr) {
	int jc, okd, okd2;
	for (int i = 0; i < num; ++i) {
		okd = 1;
		okd2 = 1;

		for (int j = 1; j < num; ++j) {
			if (board[i][j] != 0 && board[i][j - 1] == 0) {
				jc = j;
				slide_x(&jc, &i, 0, num, board);

				if (jc > 0 && board[i][jc] != 0 && board[i][jc - 1] == board[i][jc] && okd2) {
					slide_merge(&jc, &i, 0, board, scr);
					okd = !okd;
				}

			} else if (board[i][j] != 0 && board[i][j - 1] == board[i][j] && okd2) {
				jc = j;
				slide_merge(&jc, &i, 0, board, scr);

				okd = !okd;
			}

			if (okd == 0) okd2 = !okd2;
		}
	}
}

// Processes a move in the 'up' direction for the entire board (column by column).
void move_up(const int num, int board[NMAX][NMAX], int *scr) {
	int ic, okd, okd2;
	for (int j = 0; j < num; ++j) {
		okd = 1;
		okd2 = 1;

		for (int i = 1; i < num; ++i) {
			if (board[i][j] != 0 && board[i - 1][j] == 0) {
				ic = i;
				slide_y(&j, &ic, 2, num, board);

				if (ic > 0 && board[ic][j] != 0 && board[ic - 1][j] == board[ic][j] && okd2) {
					slide_merge(&j, &ic, 2, board, scr);
					okd = !okd;
				}

			} else if (board[i][j] != 0 && board[i - 1][j] == board[i][j] && okd2) {
				ic = i;
				slide_merge(&j, &ic, 2, board, scr);

				okd = !okd;
			}

			if (okd == 0) okd2 = !okd2;
		}
	}
}

// Processes a move in the 'down' direction for the entire board (column by column).
void move_down(const int num, int board[NMAX][NMAX], int *scr) {
	int ic, okd, okd2;
	for (int j = 0; j < num; ++j) {
		okd = 1;
		okd2 = 1;

		for (int i = num - 2; i >= 0; --i) {
			if (board[i][j] != 0 && board[i + 1][j] == 0) {
				ic = i;
				slide_y(&j, &ic, 3, num, board);

				if (board[ic][j] != 0 && board[ic + 1][j] == board[ic][j] && okd2) {
					slide_merge(&j, &ic, 3, board, scr);
					okd = 0;
				}

			} else if (ic < num - 1 && board[i][j] != 0 && board[i + 1][j] == board[i][j] && okd2) {
				ic = i;
				slide_merge(&j, &ic, 3, board, scr);

				okd = 0;
			}

			if (okd == 0) okd2 = !okd2;
		}
	}
}

// Returns 1 if there are any empty cells, 0 otherwise.
int is_empty_cells(const int num, int board[NMAX][NMAX]) {
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < num; ++j)
			if (board[i][j] == 0) return 1;

	return 0;
}

// Undoes the last move made by the player, keeping track of the previous score.
void undo(const int num, int board[NMAX][NMAX], const int prev_board[NMAX][NMAX], int *scr,
		  const int prev_scr) {
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < num; ++j)
			board[i][j] = prev_board[i][j];

	*scr = prev_scr;
}

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

// Processes the outcome of a move.
void process_move(const int num, int board[NMAX][NMAX], int prev_board[NMAX][NMAX],
				  int board_buffer[NMAX][NMAX], int scr, int *prev_scr, int scr_buffer, int *maxn,
				  int *ok_game, int *move_cnt) {
	if (is_move_valid(num, board, prev_board)) {
		clear_screen();
		spawn(num, board, ok_game);
		write_board(scr, num, board, maxn);
		usleep(150000);
		(*move_cnt)++;
	} else {
		build_prev_board(num, board_buffer, prev_board, scr_buffer, prev_scr);
	}
}

// Runs a game of 2048.
// The game ends if there are no possible moves left (in which case the game is lost), or if there
// is a cell that contains the value win_num (in which case the game is won).
// If resume is non-zero the board, score and undo state are loaded from the save file instead of
// starting from an empty board (num and win_num must match the values stored in the save).
void run_game(const int num, const unsigned long win_num, const int resume) {
	int board[NMAX][NMAX] = {0}, prev_board[NMAX][NMAX] = {0};
	int board_buffer[NMAX][NMAX] = {0}, scr_buffer = 0;
	int ok_game = 0, maxn = 0, scr = 0, prev_scr = 0, move_cnt = 0, saved_cnt = 0;

	if (resume) {
		int saved_num;
		unsigned long saved_win_num;

		if (load_game(&saved_num, &saved_win_num, board, prev_board, &scr, &prev_scr, &move_cnt,
					  &maxn) != 0 ||
			saved_num != num || saved_win_num != win_num) {
			fprintf(stderr, "Could not load the saved game!\n");
			return;
		}
		saved_cnt = move_cnt;
	}

	enable_view();
	clear_screen();
	if (!resume) spawn(num, board, &ok_game);
	write_board(scr, num, board, &maxn);

	while (!ok_game) {
		if (!is_empty_cells(num, board) && !is_any_possible_move(num, board)) ok_game = 1;

		if ((unsigned long)maxn >= win_num) {
			disable_raw_mode();
			delete_save();
			fprintf(stdout, "Well done! You beat my game with a score of %d, in %d moves.\n", scr,
					move_cnt);
			return;
		}

		int key = read_key();

		if (key == KEY_Q) {
			disable_raw_mode();
			if (save_game(num, win_num, board, prev_board, scr, prev_scr, move_cnt, maxn) == 0)
				fprintf(stdout, "Game saved. Resume it with `play2048 -r`.\n");
			else
				fprintf(stderr, "Could not save the game!\n");
			return;
		}

		if (key == KEY_LEFT) {
			build_prev_board(num, prev_board, board_buffer, prev_scr, &scr_buffer);
			build_prev_board(num, board, prev_board, scr, &prev_scr);
			move_left(num, board, &scr);

			process_move(num, board, prev_board, board_buffer, scr, &prev_scr, scr_buffer, &maxn,
						 &ok_game, &move_cnt);
		}
		if (key == KEY_UP) {
			build_prev_board(num, prev_board, board_buffer, prev_scr, &scr_buffer);
			build_prev_board(num, board, prev_board, scr, &prev_scr);
			move_up(num, board, &scr);

			process_move(num, board, prev_board, board_buffer, scr, &prev_scr, scr_buffer, &maxn,
						 &ok_game, &move_cnt);
		}
		if (key == KEY_DOWN) {
			build_prev_board(num, prev_board, board_buffer, prev_scr, &scr_buffer);
			build_prev_board(num, board, prev_board, scr, &prev_scr);
			move_down(num, board, &scr);

			process_move(num, board, prev_board, board_buffer, scr, &prev_scr, scr_buffer, &maxn,
						 &ok_game, &move_cnt);
		}
		if (key == KEY_RIGHT) {
			build_prev_board(num, prev_board, board_buffer, prev_scr, &scr_buffer);
			build_prev_board(num, board, prev_board, scr, &prev_scr);
			move_right(num, board, &scr);

			process_move(num, board, prev_board, board_buffer, scr, &prev_scr, scr_buffer, &maxn,
						 &ok_game, &move_cnt);
		}
		if (key == KEY_R && move_cnt != 0) {
			undo(num, board, prev_board, &scr, prev_scr);
			clear_screen();
			write_board(scr, num, board, &maxn);
			usleep(150000);
			--move_cnt;
		}

		// Autosave whenever a move (or an undo) changed the game, so progress survives a crash.
		if (move_cnt != saved_cnt) {
			save_game(num, win_num, board, prev_board, scr, prev_scr, move_cnt, maxn);
			saved_cnt = move_cnt;
		}
	}

	disable_raw_mode();
	delete_save();
	fprintf(stdout, "No possible moves left.\nYou lost!\n");
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

int main(int argc, char *argv[]) {
	int num, resume = 0;

	if (argc > 2) {
		fprintf(stderr, "Too many arguments.\n");
		print_usage(stderr);
		return 1;
	}

	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			print_usage(stdout);
			return 0;
		} else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--resume") == 0) {
			resume = 1;
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[1]);
			print_usage(stderr);
			return 1;
		}
	}

	srand((unsigned)time(NULL));

	if (resume) {
		unsigned long saved_win_num;
		int tmp_board[NMAX][NMAX], tmp_prev[NMAX][NMAX], tmp_scr, tmp_prev_scr, tmp_cnt, tmp_max;
		int load_ret = load_game(&num, &saved_win_num, tmp_board, tmp_prev, &tmp_scr, &tmp_prev_scr,
								 &tmp_cnt, &tmp_max);

		if (load_ret == 0) {
			run_game(num, saved_win_num, 1);
			return 0;
		}

		if (load_ret == -1) fprintf(stderr, "The saved game is invalid, starting a new game.\n");
	}

	fprintf(
		stdout,
		"Press the right, left, up or down arrow to move the aux_buffers in the wanted direction, "
		"press `r` to return 1 move, `q` to save and quit\n"
		"How big do you want your board to be?\n"
		"Press `enter` to confirm.\n"
		"Enter a number between 4 to 8:\n");

	int aux_read_ret = read_grid_num(&num);

	if (aux_read_ret == 1)
		return 0;
	else if (aux_read_ret == -1) {
		fprintf(stderr, "sscanf() failed!\n");
		return 1;
	}

	unsigned long win_num;
	if (read_win_num(&win_num) != 0) return 0;

	run_game(num, win_num, 0);

	return 0;
}
