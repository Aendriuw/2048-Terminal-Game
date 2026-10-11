# 2048 Terminal Game

An educational recreation of the classic 2048 game which can be played inside the terminal.

## Build

bash:
```bash
make
```

## Run

bash (in the same directory):
```bash
./2048
```

or from anywhere in the file system, if you add the executable to a path directory using `make install`:
```bash
play2048
```

## Adding the executable to the Path
- `make install` - Compiles the game (if not already done) and automatically moves a copy to your `~/.local/bin folder` as `play2048`.
- `make uninstall` - Removes the command from your system.
- `make clean` - Removes the local `.c` executable from your current project folder.

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
