//board.cpp

#include "board.h"
#include <limits>
#include <stdexcept>
using namespace std;

//define functions

//load sudoku puzzle from txt file into board array
void loadBoardFromFile(const string& filename, int board[9][9]) { //defines a function with string and array input
    ifstream file(filename);
    if (!file) throw runtime_error("Could not open puzzle: " + filename);
    int loaded[9][9] = {};
    for (auto& row : loaded) {
        for (int& value : row) {
            if (!(file >> value) || value < 0 || value > 9)
                throw runtime_error("Puzzle must contain exactly 81 integers in 0..9");
        }
    }
    string extra;
    if (file >> extra) throw runtime_error("Puzzle has extra data");
    if (!isBoardValid(loaded)) throw runtime_error("Puzzle has conflicting clues");
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) board[r][c] = loaded[r][c];
}

// display sudoku board in formatted CLI grid
void displayBoard(int board[9][9]) {
    cout << "\n==============================\n";
    cout << "          SUDOKU GAME         \n"; //nice title
    cout << "==============================\n";

    for (int i = 0; i < 9; ++i) {
        if (i % 3 == 0) //if remainder equals zero
            cout << "+-------+-------+-------+\n"; //prints row borders

        for (int j = 0; j < 9; ++j) {
            if (j % 3 == 0) cout << "| "; //prints column borders

            if (board[i][j] == 0)
                cout << "_ "; //show _ for empty cells
            else
                cout << board[i][j] << " "; //show the number
        }
        cout << "|\n"; //end-of-row line
    }
    cout << "+-------+-------+-------+\n\n"; //final bottom grid line
}

//check if placing a number is valid
bool isValidMove(int board[9][9], int row, int col, int num) {
    if (row < 0 || row >= 9 || col < 0 || col >= 9 || num < 1 || num > 9) return false;
    // check row
    for (int j = 0; j < 9; ++j) {
        if (board[row][j] == num) return false; //scan row for duplicates
    }

    // check column
    for (int i = 0; i < 9; ++i) {
        if (board[i][col] == num) return false; //scan column for duplicates
    }

    // check 3x3 subgrid
    int startRow = row - row % 3; //finds the top left of the 3x3 box of interest
    int startCol = col - col % 3;
    for (int i = 0; i < 3; ++i) //scans 3x3 box for duplicates
        for (int j = 0; j < 3; ++j)
            if (board[startRow + i][startCol + j] == num)
                return false;

    return true; //valid move if duplicates not found
}

//check if all cells are filled
bool isBoardFull(int board[9][9]) {
    for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j)
            if (board[i][j] == 0) //false if any of the cells are 0
                return false;
    return true;
}

//check if the entire board is valid (for final victory check)
bool isBoardValid(int board[9][9]) {
    //validate all filled cells
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            int num = board[i][j]; //store current number of the cell
            if (num != 0) {
                board[i][j] = 0; //temporarily clear current cell
                if (!isValidMove(board, i, j, num)) { //test if placing that number was valid
                    board[i][j] = num; 
                    return false;
                }
                board[i][j] = num; //restore value
            }
        }
    }
    return true; 
}

//allow user to select and fill a cell
void selectCell(int board[9][9]) {
    int row, col, num; //declare variables for coordinates and number

    cout << "Enter row (1-9): ";
    if (!(cin >> row)) {
        if (!cin.eof()) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); }
        cout << "Invalid numeric input.\n"; return;
    }
    cout << "Enter column (1-9): ";
    if (!(cin >> col)) {
        if (!cin.eof()) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); }
        cout << "Invalid numeric input.\n"; return;
    }

    // Validate before subtraction, including the minimum representable integer.
    if (row < 1 || row > 9 || col < 1 || col > 9) {
        cout << "Invalid position! Try again.\n";
        return;
    }


    row -= 1;
    col -= 1;

    //disallow overwriting filled cell
    if (board[row][col] != 0) {
        cout << "Cell already filled! Choose another.\n";
        return;
    }

    //ask user number input
    cout << "Enter number (1-9): ";
    if (!(cin >> num)) {
        if (!cin.eof()) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); }
        cout << "Invalid numeric input.\n"; return;
    }

    //validate number range
    if (num < 1 || num > 9) {
        cout << "Invalid number! Must be between 1 and 9.\n";
        return;
    }

    //if valid, fill cell; else print warning
    if (isValidMove(board, row, col, num)) {
        board[row][col] = num;
        cout << "Move accepted.\n";
    } else {
        cout << "Invalid move (violates Sudoku rules).\n";
    }
}