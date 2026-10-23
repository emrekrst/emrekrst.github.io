//solver.cpp
//uses backtracking with MRV

#include "board.h"
#include <limits>

using namespace std;

//helper find the cell with Minimum Remaining Values (MRV)
bool findBestCell(int board[9][9], int &row, int &col) { //new function, chooses best cell
    int minOptions = numeric_limits<int>::max(); //initialize smallest option to count to the max possible integer
    bool found = false; //finds if empty cell is found

    for (int r = 0; r < 9; r++) { // loop over every cell on the board
        for (int c = 0; c < 9; c++) {
            if (board[r][c] == 0) { //if cell is empty
                int count = 0;
                for (int num = 1; num <= 9; num++) { //count how many numbers are valid for this cell
                    if (isValidMove(board, r, c, num)) count++;
                }
                if (count < minOptions) {
                    minOptions = count;
                    row = r; //if this cell has fewer valid numbers than
                    col = c; //current min, store it as new best cell
                    found = true;
                }
            }
        }
    }
    return found; //continue scanning board until its found
}

//core solver
bool solveSudoku(int board[9][9]) {
    if (!isBoardValid(board)) return false;
    int row, col;

    if (!findBestCell(board, row, col))  //if no empty cell left
        return true; //solved

    for (int num = 1; num <= 9; num++) { //try numbers 1-9 in celected cell
        if (isValidMove(board, row, col, num)) { //if number is valid
            board[row][col] = num; //temporarily place number
            if (solveSudoku(board)) //solve the rest of the board
                return true; //and if it works its done
            board[row][col] = 0; //otherwise reset to zero and try next number (backtrack)
        }
    }
    return false; //if none worked backtrack to findBestCell step
}