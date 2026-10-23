#include "board.h"
#include <cassert>
#include <sstream>
#include <fstream>
int main(int argc, char** argv) {
    int b[9][9] = {};
    if (argc > 1 && std::string(argv[1]) == "invalid") {
        for(auto& row:b) for(auto& v:row) v=1;
        assert(!solveSudoku(b));
    } else if(argc > 1 && std::string(argv[1]) == "load") {
        bool rejected=false;
        try { loadBoardFromFile("no-such-puzzle.txt", b); } catch(const std::exception&) {rejected=true;}
        assert(rejected);
    } else {
        loadBoardFromFile("board.txt", b);
        assert(isBoardValid(b));
        assert(solveSudoku(b));
        assert(isBoardFull(b) && isBoardValid(b));
        const int expected[9][9] = {
            {5,3,4,6,7,8,9,1,2},{6,7,2,1,9,5,3,4,8},{1,9,8,3,4,2,5,6,7},
            {8,5,9,7,6,1,4,2,3},{4,2,6,8,5,3,7,9,1},{7,1,3,9,2,4,8,5,6},
            {9,6,1,5,3,7,2,8,4},{2,8,7,4,1,9,6,3,5},{3,4,5,2,8,6,1,7,9}};
        for (int r=0;r<9;++r) for(int c=0;c<9;++c) assert(b[r][c]==expected[r][c]);
        loadBoardFromFile("board.txt", b);
        b[0][2]=1; // Locally legal clue, but inconsistent with the puzzle solution.
        assert(isBoardValid(b));
        assert(!solveSudoku(b));
        assert(b[0][2]==1 && b[0][3]==0);
        assert(!isValidMove(b,-1,0,1));
        assert(!isValidMove(b,0,9,1));
        assert(!isValidMove(b,0,0,10));
        std::cout << "Sudoku solve PASS\n";
    }
}
