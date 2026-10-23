#include "motor_teleop_v2/motor_control_core.hpp"
#include "remote_mvp/remote_command.hpp"
#include <iomanip>
#include <iostream>
#include <string>

// Deterministic timestamps, no sockets, serial port, timers, or motor drivers.
int main() {
    MotorControlCore core({0.75, 1.5, 1.0, 100, false, false});
    remote_mvp::RemoteSequenceGate sequence;
    std::cout << "time_ms,accepted,left,right,command_age_ms,watchdog_stopped\n";
    std::cout << std::fixed << std::setprecision(3);
    for (int now = 0; now <= 200; now += 20) {
        std::string packet;
        if (now == 0 || now == 20) packet = "ROVER1 1 0 0.8 0\n";
        if (now == 40) packet = "ROVER1 1 1 0.8 0.2\n";
        if (now == 60) packet = "ROVER1 1 2 nan 0\n";
        if (now == 180) packet = "ROVER1 1 2 0.4 0\n";
        bool accepted = false;
        if (!packet.empty()) {
            try {
                const auto command = remote_mvp::parseRemoteCommand(packet);
                if (sequence.accept(command)) {
                    core.acceptCommand(command.linear_x, command.angular_z, now);
                    accepted = true;
                }
            } catch (const std::invalid_argument&) {
                // Malformed input cannot refresh the watchdog.
            }
        }
        const auto result = core.tick(now, 0.020);
        std::cout << now << ',' << accepted << ',' << result.applied.left << ','
                  << result.applied.right << ',' << result.command_age_ms << ','
                  << result.watchdog_stopped << '\n';
    }
}
