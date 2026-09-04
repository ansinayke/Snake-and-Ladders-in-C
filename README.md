# Maze Runner at UCSC

[![Standard: C11](https://img.shields.io/badge/Standard-C11-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Libraries: Standard C](https://shields.io)](https://wikipedia.org)
[![Compiler: GCC](https://shields.io)](https://gnu.org)

## Overview

Maze Runner at UCSC is a terminal-based, three-player maze game written in C. Players A, B, and C explore a connected three-floor board, manage movement points, navigate changing stair directions, and compete to capture the hidden flag.

The project combines deterministic simulation with emergent gameplay: the board layout is loaded from text files, while seeded randomisation controls dice rolls, cell effects, Bawana outcomes, and stair direction changes.

## What I Built

- A complete turn-based game loop for three players.
- A three-floor board renderer with coordinates and a CLI legend.
- File-driven loading for walls, stairs, poles, the flag, and the random seed.
- Movement-dice and periodic direction-dice mechanics.
- Step-by-step movement with edge, wall, and invalid-cell collision checks.
- Stairs that can change direction every five rounds and poles that move players between floors.
- Movement-point accounting with consumable cells and additive or multiplicative bonuses.
- Bawana recovery mechanics with food poisoning, disorientation, triggered movement, happy status, and random point replenishment.
- Player collision handling that resets captured players to their starting positions.
- Loop detection for players who become stuck in repeated board states.
- Separate `log.txt` and `errors.txt` output generated at runtime.

## Technical Decisions

### Why C?

C was a deliberate choice for practising the fundamentals behind game engines and simulations: explicit state management, structs and enums, arrays, pointers, file I/O, control flow, and manual validation. It keeps the rules visible and makes each state transition easy to trace in the generated log.

### Why external text files?

The board geometry is separated from the game engine. Editing `stairs.txt`, `poles.txt`, `walls.txt`, or `flag.txt` changes the scenario without changing the movement code. This improves maintainability, makes the game easier to extend, and demonstrates a clean separation between configuration and behaviour.

### Why a fixed seed?

Loading the seed from `seed.txt` makes a run reproducible. Reproducible randomness is valuable when debugging a simulation because the same sequence of dice rolls and effects can be investigated repeatedly.

### Why explicit player states?

The `Player` structure and status enum keep position, direction, movement points, recovery timers, and special conditions together. This gives the game a clear state model and makes rules such as missed turns, disorientation, and triggered movement manageable.

## Gameplay Rules

### Players

- 3 players: **A, B, and C**.
- Each player starts in the **standing area** outside the maze.
- Players need to roll a **6** on the movement dice to enter the maze.

### Dice

- **Movement Dice (1–6):** Determines how many steps a player moves.
- **Direction Dice (rolled every 4th throw):**
  - Faces: **North, East, South, West, Empty**.
  - If the result is Empty, the direction does not change.
  - Otherwise, the player changes direction to the rolled value.

### Maze Components

- **Walls (`#`):** Block movement.
- **Stairs (`w`):** Connect different floors. Directions reverse every 5 rounds.
- **Poles (`o`):** Slide players down to lower floors.
- **Bridge (`=`):** Connects left and right blocks on floor 1.
- **Flag (`F`):** Capturing it ends the game.

### Movement Rules

- Players move step-by-step in their current direction.
- If blocked by a wall or edge, the move ends and the player remains in place.
- Passing through a stair cell means the player must immediately take the stair.
- If a loop occurs with stairs or poles, the player is reset to their starting cell.
- Landing on another player captures that opponent and sends them back to start.

### Movement Points

- Each player starts with **100 movement points**.
- **Consumable cells** cost points.
- **Bonus cells** add or multiply points.
- If movement points are less than or equal to 0, the player is teleported to **Bawana**.

### Gameplay Flow

1. The game begins with all 3 players in the standing area.
2. Players take turns rolling dice:
   - Enter the maze with a 6.
   - Move according to the dice results and maze rules.
   - Apply any effects, including stairs, poles, Bawana, consumables, and bonuses.
3. A round ends after all players have rolled.
4. Stair directions shuffle every 5 rounds.
5. The game continues until a player captures the flag.

## Board Legend

| Symbol | Meaning |
| --- | --- |
| `+` | Invalid cell |
| `s` | Starting area |
| `.` | Playable game cell |
| `=` | Bridge |
| `w` | Stair |
| `o` | Pole |
| `#` | Wall |
| `F` | Flag |
| `A`, `B`, `C` | Players |

## How to Run

From the project directory:

```bash
gcc main.c -o maze_runner
./maze_runner
```

The program reads the supporting `.txt` files from the current directory. It writes the full simulation to `log.txt` and diagnostic messages to `errors.txt`.

## Project Files

| File | Purpose |
| --- | --- |
| `main.c` | Game engine, board rendering, rules, state transitions, and output |
| `stairs.txt` | Stair endpoints and floor connections |
| `poles.txt` | Pole coordinates and floor ranges |
| `walls.txt` | Wall segments for each floor |
| `flag.txt` | Flag floor and coordinates |
| `seed.txt` | Seed used to initialise random behaviour |

## Runtime Output

- Errors encountered while simulating the game are printed to `errors.txt` after execution.
- The program output is written to `log.txt` after execution.

## What This Project Strengthened

Building this game improved my ability to model a non-trivial problem as explicit state, break rules into focused functions, validate external input, and reason about interactions between multiple systems. It also gave me practical experience tracing deterministic simulations through logs, handling edge cases such as movement barriers and loops, and designing a small project that can be extended through data rather than repeated code changes.

## Future Improvements

- Replace legacy function declarations with modern C prototypes.
- Add automated tests for movement, transitions, collision handling, and Bawana effects.
- Validate coordinate ranges before storing loaded entities.
- Add command-line options for the input directory and output filenames.
- Improve portability by replacing the platform-specific `sleep` dependency with a clearer optional presentation layer.
