#include "drive_logic.hpp"

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

void expectCommand(const KeyState& keys, double left, double right,
                   const std::string& message) {
  const auto command = mixKeys(keys, 0.20);
  expectNear(command.left, left, message + " left");
  expectNear(command.right, right, message + " right");
}

}  // namespace

int main() {
  expectCommand({}, 0.0, 0.0, "neutral stops");
  expectCommand({true, false, false, false}, 1.0, 1.0, "W moves forward");
  expectCommand({false, false, true, false}, -1.0, -1.0, "S moves backward");
  expectCommand({false, true, false, false}, -1.0, 1.0, "A pivots left");
  expectCommand({false, false, false, true}, 1.0, -1.0, "D pivots right");
  expectCommand({true, true, false, false}, 0.2, 1.0, "W+A curves left");
  expectCommand({true, false, false, true}, 1.0, 0.2, "W+D curves right");
  expectCommand({false, true, true, false}, -0.2, -1.0, "S+A curves left");
  expectCommand({false, false, true, true}, -1.0, -0.2, "S+D curves right");
  expectCommand({true, false, true, false}, 0.0, 0.0, "W+S cancels");
  expectCommand({false, true, false, true}, 0.0, 0.0, "A+D cancels");
  expectCommand({true, false, true, true}, 1.0, -1.0,
                "W+S+D reduces to D");
  expectCommand({true, true, true, false}, -1.0, 1.0,
                "W+A+S reduces to A");
  expectCommand({true, true, false, true}, 1.0, 1.0,
                "W+A+D reduces to W");
  expectCommand({false, true, true, true}, -1.0, -1.0,
                "A+S+D reduces to S");
  expectCommand({true, true, true, true}, 0.0, 0.0, "all keys cancel");

  bool rejected_ratio = false;
  try {
    (void)mixKeys({}, 1.01);
  } catch (const std::invalid_argument&) {
    rejected_ratio = true;
  }
  if (!rejected_ratio) {
    std::cerr << "FAIL: inner-wheel ratios above one are rejected\n";
    ++failures;
  }
  bool rejected_nan_ratio = false;
  try {
    (void)mixKeys({}, std::numeric_limits<double>::quiet_NaN());
  } catch (const std::invalid_argument&) {
    rejected_nan_ratio = true;
  }
  if (!rejected_nan_ratio) {
    std::cerr << "FAIL: non-finite inner-wheel ratio is rejected\n";
    ++failures;
  }

  RampState ramp({1.0, 2.0});
  auto ramped = ramp.update({1.0, -1.0}, 0.1);
  expectNear(ramped.left, 0.1, "left accelerates forward");
  expectNear(ramped.right, -0.1, "right accelerates backward");

  ramp = RampState({1.0, 2.0}, {0.5, -0.5});
  ramped = ramp.update({0.0, 0.0}, 0.1);
  expectNear(ramped.left, 0.3, "left decelerates toward zero");
  expectNear(ramped.right, -0.3, "right decelerates toward zero");

  ramp = RampState({1.0, 2.0}, {0.5, 0.1});
  ramped = ramp.update({-1.0, -1.0}, 0.1);
  expectNear(ramped.left, 0.3, "reversal first decelerates");
  expectNear(ramped.right, 0.0, "reversal clamps at zero");
  ramped = ramp.update({-1.0, -1.0}, 0.1);
  expectNear(ramped.left, 0.1, "continued reversal keeps decelerating");
  expectNear(ramped.right, -0.1, "reversal accelerates after zero");

  ramp = RampState({1.0, 2.0}, {0.05, -0.05});
  ramped = ramp.update({0.0, 0.0}, 0.1);
  expectNear(ramped.left, 0.0, "deceleration cannot overshoot zero");
  expectNear(ramped.right, 0.0, "negative deceleration cannot overshoot zero");

  ramp = RampState({1.0, 2.0}, {0.8, -0.7});
  ramped = ramp.emergencyStop();
  expectNear(ramped.left, 0.0, "emergency stop clears left immediately");
  expectNear(ramped.right, 0.0, "emergency stop clears right immediately");

  bool rejected_dt = false;
  try {
    (void)ramp.update({}, 0.0);
  } catch (const std::invalid_argument&) {
    rejected_dt = true;
  }
  if (!rejected_dt) {
    std::cerr << "FAIL: non-positive ramp time is rejected\n";
    ++failures;
  }

  bool rejected_rates = false;
  try {
    RampState invalid({0.0, 1.0});
    (void)invalid;
  } catch (const std::invalid_argument&) {
    rejected_rates = true;
  }
  if (!rejected_rates) {
    std::cerr << "FAIL: non-positive ramp rates are rejected\n";
    ++failures;
  }
  bool rejected_nan_rate = false;
  try {
    RampState invalid({std::numeric_limits<double>::quiet_NaN(), 1.0});
    (void)invalid;
  } catch (const std::invalid_argument&) {
    rejected_nan_rate = true;
  }
  if (!rejected_nan_rate) {
    std::cerr << "FAIL: non-finite ramp rates are rejected\n";
    ++failures;
  }

  if (failures != 0) {
    std::cerr << failures << " assertion(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "drive logic tests passed\n";
  return EXIT_SUCCESS;
}
