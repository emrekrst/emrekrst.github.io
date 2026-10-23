CXX = c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic
.PHONY: all test clean
all: build/decoder
build:
	mkdir -p build
build/decoder: main.cpp Matrix.cpp NeuralNetwork.cpp Matrix.h NeuralNetwork.h | build
	$(CXX) $(CXXFLAGS) main.cpp Matrix.cpp NeuralNetwork.cpp -o $@
build/tests: tests.cpp Matrix.cpp NeuralNetwork.cpp Matrix.h NeuralNetwork.h | build
	$(CXX) $(CXXFLAGS) -fno-access-control tests.cpp Matrix.cpp NeuralNetwork.cpp -o $@
test: all build/tests
	./build/tests
	./build/tests gradient
	./build/decoder > build/training.log
	tail -n 1 build/training.log
clean:
	rm -rf build
