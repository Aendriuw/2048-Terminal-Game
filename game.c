// Milosoiu Andrei
// Email: andrei18.milosoiu@gmail.com

#include "game.h"

#include <stdlib.h>

static const int vi[4] = {0, 0, -1, 1};
static const int vj[4] = {-1, 1, 0, 0};
const unsigned long win_options[WIN_OPTIONS_CNT] = {2048, 4096, 8192, 16384, 32768, 65536, 131072};

// Copies matrix every value of matrix src into matrix dest.
void copy_matrix(const int num, const int src[NMAX][NMAX], int dest[NMAX][NMAX]) {
	for (int i = 0; i < num; ++i) {
		for (int j = 0; j < num; ++j) {
			dest[i][j] = src[i][j];
		}
	}
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

// Returns a random digit between 2 and 4.
// 4 has a chance of 1/8 to be returned, while 2 has a chance of 7/8.
int random_digit() {
	int chn = rand() % 8;

	if (chn == 0)
		return 4;
	else
		return 2;
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
static void slide_x(int *j, int *i, int curr_dir, const int num, int board[NMAX][NMAX]) {
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
static void slide_y(int *j, int *i, int curr_dir, const int num, int board[NMAX][NMAX]) {
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
static void slide_merge(int *j, int *i, int curr_dir, int board[NMAX][NMAX], int *scr) {
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
