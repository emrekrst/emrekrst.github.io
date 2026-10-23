#include "rover_move.hpp"

int main() {
    MachineContext ctx;
    DriveState     state = DriveState::BOOT;

    while (true) {
        // ~25 Hz loop cadence; matches the Pi TX rate
        sleep_ms(40);

        bool got_letter = false;
        if (state != DriveState::BOOT) {
            got_letter = updateContextFromSerial(ctx);
        }

        switch (state) {

            // ---------------------------------------------------
            case DriveState::BOOT:
            // ---------------------------------------------------
                initHardware(ctx);
                state = DriveState::WAIT_LINK;
                break;

            // ---------------------------------------------------
            case DriveState::WAIT_LINK:
            // ---------------------------------------------------
                // Motors already zeroed by initHardware.
                // Advance as soon as the first valid letter arrives.
                if (got_letter) {
                    state = DriveState::ACTIVE_DRIVE;
                }
                break;

            // ---------------------------------------------------
            case DriveState::ACTIVE_DRIVE:
            // ---------------------------------------------------
                driveFromLetter(ctx, ctx.last_letter);
                if (isLinkTimedOut(ctx)) {
                    stopMotors(ctx);
                    state = DriveState::FAILSAFE_STOP;
                }
                break;

            // ---------------------------------------------------
            case DriveState::FAILSAFE_STOP:
            // ---------------------------------------------------
                // Motors are stopped.
                // Return to ACTIVE_DRIVE the moment a fresh valid
                // letter is received on the UART.
                if (got_letter) {
                    state = DriveState::ACTIVE_DRIVE;
                }
                break;
        }
    }

    return 0;
}
