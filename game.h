// Milosoiu Andrei
// Email: andrei18.milosoiu@gmail.com

#ifndef GAME_H
#define GAME_H

#define NMAX 8
#define FREE_POS_MAX 128

extern const unsigned long win_options[];
#define WIN_OPTIONS_CNT 7

// Copies matrix every value of matrix src into matrix dest.
void copy_matrix(const int num, const int src[NMAX][NMAX], int dest[NMAX][NMAX]);

// Returns 1 if the given number is a power of two that appears in win_options, 0 otherwise.
int is_valid_win_num(const unsigned long val);

// Returns a random digit between 2 and 4.
// 4 has a chance of 1/8 to be returned, while 2 has a chance of 7/8.
int random_digit();

// Builds the prev_board board needed for the undo function and memorises the previous score.
void build_prev_board(const int num, int board[NMAX][NMAX], int prev_board[NMAX][NMAX],
					  const int scr, int *prev_scr);

// Compares the current board with it's previous state (prev_board).
// Returns 1 if any cell differs (a move had any effect), 0 otherwise.
int is_move_valid(const int num, int board[NMAX][NMAX], int prev_board[NMAX][NMAX]);

// Returns 1 if there are any possible moves left, 0 otherwise.
int is_any_possible_move(const int num, int board[NMAX][NMAX]);

// Spawns a new number in a random free cell.
// The free cells' coordinates are memorised inside an array.
void spawn(int n, int board[NMAX][NMAX], int *ok_game);

// Processes a move in the 'right' direction for the entire board (row by row).
void move_right(const int num, int board[NMAX][NMAX], int *scr);

// Processes a move in the 'left' direction for the entire board (row by row).
void move_left(const int num, int board[NMAX][NMAX], int *scr);

// Processes a move in the 'up' direction for the entire board (column by column).
void move_up(const int num, int board[NMAX][NMAX], int *scr);

// Processes a move in the 'down' direction for the entire board (column by column).
void move_down(const int num, int board[NMAX][NMAX], int *scr);

// Returns 1 if there are any empty cells, 0 otherwise.
int is_empty_cells(const int num, int board[NMAX][NMAX]);

// Undoes the last move made by the player, keeping track of the previous score.
void undo(const int num, int board[NMAX][NMAX], const int prev_board[NMAX][NMAX], int *scr,
		  const int prev_scr);

#endif
