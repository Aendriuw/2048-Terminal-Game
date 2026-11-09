# Define the compiler and compiler flags
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Define the target executable and source file
TARGET = 2048-terminal-game
SRC = 2048-terminal-game.c game.c options_save.c
HDR = game.h options_save.h

# Installation settings
INSTALL_DIR = $(HOME)/.local/bin
COMMAND_NAME = play2048

# Default rule to build the target
all: $(TARGET)

# Rule to compile the source file into the executable
$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Rule to install the game to your PATH
install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/$(COMMAND_NAME)
	chmod +x $(INSTALL_DIR)/$(COMMAND_NAME)
	@echo "Installed successfully! You can now run '$(COMMAND_NAME)' from anywhere."

# Rule to remove the game from your PATH
uninstall:
	rm -f $(INSTALL_DIR)/$(COMMAND_NAME)
	@echo "Uninstalled $(COMMAND_NAME)."

# Rule to clean up the compiled executable in the current folder
clean:
	rm -f $(TARGET)

# Mark rules that don't represent physical files as "phony"
.PHONY: all clean install uninstall