# Tetris Game in C++ (Terminal Version)

A simple terminal-based Tetris game built in C++ using a custom game engine. It features its own rendering, input, and physics systems designed specifically to work in the Linux terminal. The game uses raw mode input handling and ASCII graphics for an immersive retro gameplay experience.

## Features

* **Tetromino Shapes**: Includes the 7 classic tetrominoes (I, J, L, O, S, T, Z).
* **Custom Engine Components**:

  * `GameObject`: Holds the game state and logic.
  * `InputSystem`: Handles non-blocking keyboard input.
  * `RenderSystem`: Draws the playfield, tetrominoes, score, and controls.
  * `PhysicsSystem`: Manages collision and line clearing.
* **Terminal Raw Mode**: Disables line buffering and echo for real-time input.
* **Score and Leveling**: Tracks and displays score, cleared lines, and level progression.
* **Cross-platform (Linux)**: Built specifically for Linux terminal environments.

## Controls

* **Left Arrow** - Move Left
* **Right Arrow** - Move Right
* **Down Arrow** - Move Down faster
* **Z** - Rotate piece

## Requirements

* Linux system
* g++ compiler

## Compilation

```bash
g++ -std=c++11 -o tetris main.cpp -lpthread
```

## Running the Game

```bash
./tetris
```

## Terminal Compatibility

The game uses ANSI escape codes to control the terminal, so it must be run directly in a compatible Linux terminal.

## License

This project is open-source and free to use for educational and personal purposes.

---

## Summary Comments for the Code

```cpp
// Tetromino definitions - shapes of all 7 Tetris pieces
// GameObject - holds all game-specific variables like position, speed, score, level, etc.
// InputSystem - handles real-time input from arrow keys and Z for rotation
// RenderSystem - draws field, pieces, score, and control instructions using wide characters
// PhysicsSystem - checks collisions and handles line clearing logic
// EnableRawMode/DisableRawMode - put terminal into raw input mode for non-blocking input
// GameEngine class - initializes game, processes input, renders graphics, checks logic
// Rotate function - rotates tetrominoes based on rotation index (0–3)
// Main game loop (in main file) - handles timing, updates game state, and re-renders
```

You can extend this base by adding music (via system beep or other audio library), increasing difficulty over time, or saving high scores to a file.
