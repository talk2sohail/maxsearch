# MaxSearch : Spotlight Search Alternative

A lightweight, blazing-fast alternative to macOS Spotlight Search, built with C++ and Raylib. 

This project aims to provide a minimal, transparent, and beautiful search interface that runs instantly.

## Features
- **Instant Startup:** Written in C++ for native performance.
- **Glassy UI:** Semi-transparent, "Tokyo Night" themed interface.
- **Minimalist:** No bloat, just a search bar that does what you need.
- **Draggable:** Click and drag the bar anywhere on your screen.

## Dependencies

You need **Raylib** installed on your system.

### macOS (Homebrew)
```bash
brew install raylib
```

## Building

A `Makefile` is provided for easy compilation.

To build the Spotlight application:
```bash
make 
```

## Controls
- **Typing:** Search for text (currently in UI demo phase).
- **Backspace:** Delete characters.
- **Drag:** Click anywhere on the bar to move it.
- **Esc:** Close the application.
