# Sudoku solver

A C++ command-line Sudoku game with a backtracking solver that chooses the empty cell with the fewest legal candidates (minimum remaining values). The supplied text puzzle is included.

## Build and run

Requires a C++17 compiler, Make and Python 3 for CLI tests.

```sh
make
./build/sudoku                 # use board.txt from this directory
./build/sudoku path/to/puzzle.txt
make test
make clean
```

Enter `s` to select an empty cell, `a` to display a solution without changing the current board, `v` to check completion, or `q` to quit. Row/column coordinates are 1–9. A puzzle file must have exactly 81 integers in 0–9; zero means empty. Invalid, missing or conflicting puzzles return a nonzero exit status.

## Provenance and preparation changes

Recovered university coursework associated with Emre Karausta. This project is an additional programming example; it is not listed in the master CV. The standalone copy retains the original board representation, CLI and MRV solver. Preparation added input validation, atomic puzzle loading, invalid-board rejection, safe EOF handling and regression tests. Original coursework files were left untouched.

No project-wide licence has been inferred or added. Confirm rights to any supplied coursework material before public release.

See [VERIFICATION.md](VERIFICATION.md) for executed checks and limits.
