#include "motor_teleop_v2/motor_control_core.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void expectNear(double actual, double expected, const std::string& message) {
  if (std::fabs(actual - expected) > 1e-9) {
    std::cerr << "FAIL: " << message << ": expected " << expected
              << ", got " << actual << '\n';
    ++failures;
  }
}

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

MotorControlConfig fastConfig() {
  return {100.0, 100.0, 1.0, 100, false, false};
}

MotorTickResult command(double linear, double angular) {
  MotorControlCore core(fastConfig());
  core.acceptCommand(linear, angular, 0);
  return core.tick(0, 0.02);
}

template <typename Function>
void expectInvalid(Function function, const std::string& message) {
  try {
    function();
    std::cerr << "FAIL: " << message << " was accepted\n";
    ++failures;
  } catch (const std::invalid_argument&) {
  }
}

}  // namespace

int main() {
  MotorControlCore startup(fastConfig());
  auto result = startup.tick(0, 0.02);
  expect(result.watchdog_stopped, "startup is watchdog-stopped");
  expectNear(result.applied.left, 0.0, "startup left is zero");
  expectNear(result.applied.right, 0.0, "startup right is zero");
  expect(result.command_age_ms == std::numeric_limits<std::uint32_t>::max(),
         "startup command age is unknown");

  result = command(1.0, 0.0);
  expectNear(result.applied.left, 1.0, "forward left");
  expectNear(result.applied.right, 1.0, "forward right");
  result = command(-1.0, 0.0);
  expectNear(result.applied.left, -1.0, "backward left");
  expectNear(result.applied.right, -1.0, "backward right");
  result = command(0.0, 1.0);
  expectNear(result.applied.left, -1.0, "positive angular pivots left");
  expectNear(result.applied.right, 1.0, "positive angular right wheel");
  result = command(0.0, -1.0);
  expectNear(result.applied.left, 1.0, "negative angular left wheel");
  expectNear(result.applied.right, -1.0, "negative angular pivots right");

  result = command(1.0, 1.0);
  expectNear(result.applied.left, 0.0, "forward-left inner wheel");
  expectNear(result.applied.right, 1.0, "forward-left outer wheel normalized");
  result = command(1.0, -1.0);
  expectNear(result.applied.left, 1.0, "forward-right outer wheel normalized");
  expectNear(result.applied.right, 0.0, "forward-right inner wheel");
  result = command(-1.0, 1.0);
  expectNear(result.applied.left, -1.0, "backward-left outer magnitude");
  expectNear(result.applied.right, 0.0, "backward-left inner wheel");
  result = command(-1.0, -1.0);
  expectNear(result.applied.left, 0.0, "backward-right inner wheel");
  expectNear(result.applied.right, -1.0, "backward-right outer magnitude");

  MotorControlCore limited({100.0, 100.0, 0.5, 100, false, false});
  limited.acceptCommand(1.0, 0.0, 0);
  result = limited.tick(0, 0.02);
  expectNear(result.applied.left, 0.5, "maximum command limits left");
  expectNear(result.applied.right, 0.5, "maximum command limits right");

  MotorControlCore inverted({100.0, 100.0, 1.0, 100, true, false});
  inverted.acceptCommand(1.0, 0.0, 0);
  result = inverted.tick(0, 0.02);
  expectNear(result.applied.left, -1.0, "left inversion is physical only");
  expectNear(result.applied.right, 1.0, "right remains uninverted");

  MotorControlCore ramped({1.0, 2.0, 1.0, 1000, false, false});
  ramped.acceptCommand(1.0, 0.0, 0);
  result = ramped.tick(0, 0.5);
  expectNear(result.applied.left, 0.5, "normal command ramps up");
  ramped.acceptCommand(0.0, 0.0, 500);
  result = ramped.tick(500, 0.1);
  expectNear(result.applied.left, 0.3, "stick center ramps down");

  MotorControlCore watchdog(fastConfig());
  watchdog.acceptCommand(1.0, 0.0, 0);
  result = watchdog.tick(99, 0.02);
  expect(!result.watchdog_stopped, "99 ms command remains fresh");
  expect(result.command_age_ms == 99, "fresh command age is reported");
  result = watchdog.tick(100, 0.02);
  expect(result.watchdog_stopped, "100 ms command expires");
  expectNear(result.applied.left, 0.0, "watchdog immediately clears left");
  expectNear(result.applied.right, 0.0, "watchdog immediately clears right");
  watchdog.acceptCommand(0.5, 0.0, 101);
  result = watchdog.tick(101, 0.02);
  expect(!result.watchdog_stopped, "new command recovers watchdog");
  expectNear(result.applied.left, 0.5, "recovery begins from stopped state");

  expectInvalid(
      [&] {
        MotorControlCore core(fastConfig());
        core.acceptCommand(std::numeric_limits<double>::quiet_NaN(), 0.0, 0);
      },
      "NaN command");
  expectInvalid(
      [&] {
        MotorControlCore core(fastConfig());
        core.acceptCommand(0.0, 0.0, 100);
        (void)core.tick(99, 0.02);
      },
      "clock reversal");
  expectInvalid(
      [] { MotorControlCore core({1.0, 1.0, 0.0, 100, false, false}); },
      "zero command limit");
  expectInvalid(
      [] { MotorControlCore core({1.0, 1.0, 1.0, 39, false, false}); },
      "unsafe watchdog");

  if (failures != 0) {
    std::cerr << failures << " assertion(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "motor control core tests passed\n";
  return EXIT_SUCCESS;
}
