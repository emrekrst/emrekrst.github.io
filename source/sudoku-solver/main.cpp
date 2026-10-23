//main.cpp

#include <iostream>
#include <string>
#include "board.h"

int main(int argc, char** argv) {
    int board[9][9] = {}; //define 9x9 2D array for board number representations
    std::string filename = argc > 1 ? argv[1] : "board.txt"; //string defines a sequence of characters which is the name of the file here
    try { loadBoardFromFile(filename, board); }
    catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } // //use filename string to get information from the txt file to the 9x9 array

    bool running = true; //to create an infinite loop
    while (running) { //loop will always run
        displayBoard(board); //shows the board with visuals
        char command; //define a one letter char of which command is selected
        std::cout << "Enter command (s = select, a = autosolve/answer, v = validate, q = quit): "; //shows this to user from the terminal
        if (!(std::cin >> command)) break; // //lets user enter a one letter input to be the next command

        switch (command) { //this is like an if-else loop that just switches to the case whatever the command was by the user
            case 's':
                selectCell(board); //allows user to select the cell and place 1-9 number to the cell
                break;
            case 'a':
                int solved[9][9]; //defines a new array for solution
                for (int i = 0; i < 9; ++i) {
                    for (int j = 0; j < 9; ++j) {
                        solved[i][j] = board[i][j]; //prints the board to the solved array
                    }
                }
                if (solveSudoku(solved)) { //uses the algorithm for this seperate array to fill in the answers
                    std::cout << "Solution:\n"; //print this message
                    displayBoard(solved); //display the solved board
                } else {
                    std::cout << "No solution exists.\n"; //to show when algorithm fails to find a solution
                }
                break;
            case 'v':
                if (isBoardFull(board)) { //checks if board is full
                    if (isBoardValid(board)) { 
                        std::cout << "Congratulations! Puzzle solved!\n"; //prints this if solved
                        running = false; //ends game loop
                    } else {
                        std::cout << "Some cells are incorrect.\n"; //prints this if answers are incorrect, the board must be full to see this
                    }
                } else {
                    std::cout << "The board is not complete yet. Keep going!\n"; //isBoardFull function gives not true
                }
                break;
            case 'q':
                running = false; //directly ends the game loop
                break;
            default: //if none of the valid command is entered
                std::cout << "Invalid command.\n";
        }
    }
    return 0;
}