# Pico firmware

See the repository [README](../../README.md) for SDK configuration and tests. The default board is RP2040 Pico. `rover_move.cpp` implements boot, waiting, active drive and failsafe states; `rover_move.hpp` contains parser, steering and hardware primitives.

UART0 RX is GP17, TX GP16, at 9600 baud. M1 DIR/PWM is GP1/GP0; M2 DIR/PWM GP10/GP11. The control loop runs every 40 ms, the slew step is 8, base demand 120/255, the K-P/k-p band is straight, and timeout is greater than 300 ms since the last valid line. Validate these settings against the actual rover. Native tests do not prove wiring, power, motor direction, stopping distance or mechanical safety.
