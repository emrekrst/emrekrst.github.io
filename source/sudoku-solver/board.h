//board.h

#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <fstream>
#include <string>

// SUDOKU FUNCTIONS

// Loads the Sudoku puzzle from a text file into the 9x9 board array
void loadBoardFromFile(const std::string& filename, int board[9][9]);

// Displays the current Sudoku board in a formatted grid
void displayBoard(int board[9][9]);

// Checks whether a number can be legally placed at a given position
bool isValidMove(int board[9][9], int row, int col, int num);

// Checks if all cells are filled (no zero values)
bool isBoardFull(int board[9][9]);

// Checks whether the Sudoku board is completely valid (used to confirm victory)
bool isBoardValid(int board[9][9]);

// Allows the user to select a cell and enter a number
void selectCell(int board[9][9]);

// Sudoku solver to give the answer when the user wants access
bool solveSudoku(int board[9][9]); 

#endif // BOARD_H