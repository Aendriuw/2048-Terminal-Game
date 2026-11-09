// Milosoiu Andrei
// Email: andrei18.milosoiu@gmail.com

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "options_save.h"

#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_UP 3
#define KEY_DOWN 4
#define KEY_R 5
#define KEY_Q 6
#define NO_KEY 0

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

int main(int argc, char *argv[]) {
	int num, resume;

	int opt_ret = parse_options(argc, argv, &resume);
	if (opt_ret == OPT_EXIT_OK) return 0;
	if (opt_ret == OPT_EXIT_ERR) return 1;

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
