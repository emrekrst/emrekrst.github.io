#pragma once

#include <stdexcept>
#include <algorithm>
#include <cmath>

struct KeyState {
  bool w{};
  bool a{};
  bool s{};
  bool d{};
};

struct WheelCommand {
  double left{};
  double right{};
};

inline WheelCommand mixKeys(const KeyState& keys, double inner_ratio) {
  if (!std::isfinite(inner_ratio) || inner_ratio < 0.0 || inner_ratio > 1.0) {
    throw std::invalid_argument("inner ratio must be between zero and one");
  }
  const int translation = static_cast<int>(keys.w) - static_cast<int>(keys.s);
  const int turn = static_cast<int>(keys.d) - static_cast<int>(keys.a);

  if (translation == 0 && turn == 0) {
    return {};
  }
  if (translation == 0) {
    return {static_cast<double>(turn), static_cast<double>(-turn)};
  }
  if (turn == 0) {
    return {static_cast<double>(translation), static_cast<double>(translation)};
  }

  const double direction = static_cast<double>(translation);
  if (turn > 0) {
    return {direction, direction * inner_ratio};
  }
  return {direction * inner_ratio, direction};
}

struct RampConfig {
  double acceleration_per_second{};
  double deceleration_per_second{};
};

class RampState {
 public:
  explicit RampState(RampConfig config, WheelCommand initial = {})
      : config_(config), current_(initial) {
    if (!std::isfinite(config_.acceleration_per_second) ||
        !std::isfinite(config_.deceleration_per_second) ||
        config_.acceleration_per_second <= 0.0 ||
        config_.deceleration_per_second <= 0.0) {
      throw std::invalid_argument("ramp rates must be positive");
    }
  }

  WheelCommand update(WheelCommand target, double elapsed_seconds) {
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds <= 0.0 ||
        !std::isfinite(target.left) || !std::isfinite(target.right)) {
      throw std::invalid_argument("ramp elapsed time must be positive");
    }
    current_.left = updateWheel(current_.left, target.left, elapsed_seconds);
    current_.right = updateWheel(current_.right, target.right, elapsed_seconds);
    return current_;
  }

  WheelCommand emergencyStop() {
    current_ = {};
    return current_;
  }

  WheelCommand current() const { return current_; }

 private:
  static double moveToward(double current, double target, double maximum_delta) {
    if (current < target) {
      return std::min(current + maximum_delta, target);
    }
    return std::max(current - maximum_delta, target);
  }

  double updateWheel(double current, double target, double elapsed_seconds) const {
    if (current * target < 0.0) {
      return moveToward(current, 0.0,
                        config_.deceleration_per_second * elapsed_seconds);
    }
    const bool increasing_magnitude = std::fabs(target) > std::fabs(current);
    const double rate = increasing_magnitude
                            ? config_.acceleration_per_second
                            : config_.deceleration_per_second;
    return moveToward(current, target, rate * elapsed_seconds);
  }

  RampConfig config_;
  WheelCommand current_;
};
