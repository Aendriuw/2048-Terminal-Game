# 2048 Terminal Game

An educational recreation of the classic 2048 game which can be played inside the terminal.

## Build

bash:
`gcc -o 2048 2048-terminal-game.c`

## Run

bash:
`./2048`

## How to play

1. Use the `arrow keys` to slide the tiles in the desired direction.
2. When 2 tiles of the same value collide (one slides into the other) there remains a single tile with the value of the sum of those 2. 
3. Use the `r` key to undo one move.
4. A new tile of `2` or `4` spawns in a free space after every valid move.
5. The game is won when the number `2048` is created.
6. The game is lost when there are no possible moves left.

## Screenshots of the game

<img width="1256" height="834" alt="Screenshot from 2026-02-22 14-17-28" src="https://github.com/user-attachments/assets/d142e2dd-40f7-492d-8b5d-eec9fdf59a28" />

<img width="1343" height="704" alt="Screenshot from 2026-02-22 14-18-17" src="https://github.com/user-attachments/assets/1448e518-5de1-46f4-b043-dc48fa5e2fe7" />
