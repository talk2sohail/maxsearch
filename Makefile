# Odin Makefile

APP_NAME = maxsearch
SRC_DIR = .
OUT_DIR = bin

# Ensure output directory exists
$(shell mkdir -p $(OUT_DIR))

# Compiler flags
# -debug: Generate debug info (creates .dSYM on macOS)
# -o:speed: Optimize for speed (use for release builds)
FLAGS = -debug

.PHONY: all build run clean

all: build

# Build the executable
# We build the directory $(SRC_DIR) because main.odin and search_engine.odin are there
build:
	odin build $(SRC_DIR) -out:$(OUT_DIR)/$(APP_NAME) $(FLAGS)

# Build and run
run: build
	./$(OUT_DIR)/$(APP_NAME)

# Release build (optimized)
release:
	odin build $(SRC_DIR) -out:$(OUT_DIR)/$(APP_NAME) -o:speed

# Clean up
clean:
	rm -rf $(OUT_DIR)
	rm -rf *.dSYM
	rm -rf $(SRC_DIR)/*.dSYM
