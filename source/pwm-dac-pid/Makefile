CXX = c++
CC = cc
AVR_CC = avr-gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror
.PHONY: all test firmware clean
all: build/pwm_cli
build:
	mkdir -p build
build/protocol.o: protocol.c protocol.h | build
	$(CC) $(CFLAGS) -c protocol.c -o $@
build/pwm_cli: pwm_cli.cpp protocol.h build/protocol.o
	$(CXX) $(CXXFLAGS) pwm_cli.cpp build/protocol.o -o $@
build/test_protocol: tests/test_protocol.c protocol.c protocol.h | build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -I. tests/test_protocol.c protocol.c -o $@
firmware: | build
	$(AVR_CC) -mmcu=atmega644p -DF_CPU=12000000UL -Os -Wall -Wextra -Werror embedded_skeleton.c protocol.c -o build/firmware.elf
test: all build/test_protocol
	./build/test_protocol
	python3 tests/test_gui.py
	python3 tests/test_serial.py
	python3 -m py_compile gui.py gui_plus.py
clean:
	rm -rf build __pycache__ tests/__pycache__
