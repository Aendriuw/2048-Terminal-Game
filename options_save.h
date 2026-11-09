// Milosoiu Andrei
// Email: andrei18.milosoiu@gmail.com

#ifndef OPTIONS_SAVE_H
#define OPTIONS_SAVE_H

#include <stdio.h>

#include "game.h"

#define OPT_RUN 0
#define OPT_EXIT_OK 1
#define OPT_EXIT_ERR 2

// Writes into path the location of the save file, following the XDG convention
// ($XDG_DATA_HOME/play2048/save, or ~/.local/share/play2048/save).
// If create_dir is non-zero the directories leading to the file are created too.
// Returns 0 on success, 1 on failure.
int get_save_path(char *path, const size_t len, const int create_dir);

// Saves the current game state (including the state needed for undo) to the save file.
// The data is written to a temporary file first and then renamed, so an interrupted save
// can never corrupt an existing one.
// Returns 0 on success, 1 on failure.
int save_game(const int num, const unsigned long win_num, const int board[NMAX][NMAX],
			  const int prev_board[NMAX][NMAX], const int scr, const int prev_scr,
			  const int move_cnt, const int maxn);

// Loads a game state from the save file. The output parameters are only modified on success.
// Returns 0 on success, 1 if there is no save file, -1 if the save file is invalid.
int load_game(int *num, unsigned long *win_num, int board[NMAX][NMAX], int prev_board[NMAX][NMAX],
			  int *scr, int *prev_scr, int *move_cnt, int *maxn);

// Deletes the save file, if there is one.
void delete_save();

// Prints the usage of the program to the given stream.
void print_usage(FILE *out);

// Parses the command line arguments.
// Sets *resume to 1 if the saved game should be resumed (-r, --resume).
// Returns OPT_RUN if the program should continue, OPT_EXIT_OK if it should exit successfully
// (e.g. after printing the help) and OPT_EXIT_ERR if the arguments are invalid.
int parse_options(const int argc, char *argv[], int *resume);

#endif
