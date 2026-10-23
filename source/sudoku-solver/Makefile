CXX = c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic
.PHONY: all test clean
all: build/sudoku
build:
	mkdir -p build
build/sudoku: main.cpp board.cpp solver.cpp board.h | build
	$(CXX) $(CXXFLAGS) main.cpp board.cpp solver.cpp -o $@
build/tests: tests.cpp board.cpp solver.cpp board.h | build
	$(CXX) $(CXXFLAGS) tests.cpp board.cpp solver.cpp -o $@
test: all build/tests
	./build/tests invalid
	./build/tests load
	./build/tests
	python3 test_cli.py
clean:
	rm -rf build
