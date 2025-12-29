# MaxSearch (Odin)

A lightweight, blazing-fast macOS Spotlight Search alternative, now written in **Odin** using **Raylib**.

This project provides a minimal, transparent, and beautiful search interface that queries the macOS metadata index (Spotlight) directly.

## Features

-   **Native Performance:** Written in Odin for modern, high-performance systems programming.
-   **Async Search:** Non-blocking search queries using a background thread and `mdfind` (Spotlight CLI).
-   **Smart Ranking:** Prioritizes Applications (`.app`) and shorter paths.
-   **Visuals:** "Tokyo Dark" themed, semi-transparent "glass" UI.
-   **UX:** Tab completion, "Open" hints, and draggable window.

## Prerequisites

You need the **Odin** compiler installed.

### macOS
```bash
brew install odin
```

*(Raylib is included as a vendor library in Odin, so no separate installation is required!)*

## Running

The project is located in the `maxsearch` directory.

### Quick Start
```bash
cd maxsearch
make run
```

### Build Commands

You can use the provided `Makefile`:

-   **Run:** `make run`
-   **Build:** `make build`
-   **Clean:** `make clean`


## Controls

-   **Typing:** Search for files or apps (e.g., "chrome", "notes").
-   **Tab:** Autocomplete the suggested filename.
-   **Backspace:** Delete characters (hold `Alt` to delete words).
-   **Drag:** Click anywhere on the bar to move the window.
-   **Esc:** Close the application.