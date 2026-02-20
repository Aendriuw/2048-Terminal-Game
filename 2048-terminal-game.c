// Milosoiu Andrei
// Email: andrei18.milosoiu@gmail.com

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define NMAX 8
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_UP 3
#define KEY_DOWN 4
#define KEY_R 5
#define NO_KEY 0
#define FREE_POS_MAX 128

const int vi[4] = {0, 0, -1, 1};
const int vj[4] = {-1, 1, 0, 0};

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
// is a cell that contains the value 2048 (in which case the game is won).
void run_game(const int num) {
	int board[NMAX][NMAX] = {0}, prev_board[NMAX][NMAX] = {0}, free_pos[FREE_POS_MAX];
	int board_buffer[NMAX][NMAX] = {0}, scr_buffer = 0;
	int ok_game = 0, maxn = 0, scr = 0, prev_scr = 0, move_cnt = 0;

	enable_view();
	clear_screen();
	spawn(num, board, &ok_game);
	write_board(scr, num, board, &maxn);

	while (!ok_game) {
		if (!is_empty_cells(num, board) && !is_any_possible_move(num, board)) ok_game = 1;

		if (maxn == 2048) {
			disable_raw_mode();
			fprintf(stdout, "Well done! You beat my game with a score of %d, in %d moves.\n", scr,
					move_cnt);
			return;
		}

		int key = read_key();

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
	}

	disable_raw_mode();
	fprintf(stdout, "No possible moves left.\nYou lost!\n");
}

int main(void) {
	int num;
	srand((unsigned)time(NULL));

	fprintf(
		stdout,
		"How big do you want your board to be?\n"
		"Press the right, left, up or down arrow to move the aux_buffers in the wanted direction, "
		"press r to return 1 move\n"
		"Press enter to confirm.\n"
		"Enter a aux_buffer between 4 to 8:\n");

	int aux_read_ret = read_grid_num(&num);

	if (aux_read_ret == 1)
		return 0;
	else if (aux_read_ret == -1) {
		fprintf(stderr, "sscanf() failed!\n");
		return 1;
	}

	run_game(num);

	return 0;
}