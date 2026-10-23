#pragma once

#include "drive_logic.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

struct MotorControlConfig {
  double acceleration_per_second{};
  double deceleration_per_second{};
  double maximum_command{1.0};
  std::int64_t watchdog_ms{100};
  bool invert_left{};
  bool invert_right{};
};

struct MotorTickResult {
  WheelCommand applied;
  double requested_linear_x{};
  double requested_angular_z{};
  std::uint32_t command_age_ms{std::numeric_limits<std::uint32_t>::max()};
  bool watchdog_stopped{true};
};

class MotorControlCore {
 public:
  explicit MotorControlCore(MotorControlConfig config)
      : config_(config),
        ramp_({config.acceleration_per_second,
               config.deceleration_per_second}) {
    if (!std::isfinite(config_.maximum_command) ||
        config_.maximum_command <= 0.0 || config_.maximum_command > 1.0) {
      throw std::invalid_argument("maximum command must be in (0, 1]");
    }
    if (config_.watchdog_ms < 40) {
      throw std::invalid_argument("watchdog must be at least 40 ms");
    }
  }

  void acceptCommand(double linear_x, double angular_z,
                     std::int64_t now_ms) {
    if (!std::isfinite(linear_x) || !std::isfinite(angular_z)) {
      throw std::invalid_argument("motion command must be finite");
    }
    if (now_ms < 0) {
      throw std::invalid_argument("command time cannot be negative");
    }
    requested_linear_x_ = linear_x;
    requested_angular_z_ = angular_z;
    last_command_ms_ = now_ms;
    has_command_ = true;
  }

  MotorTickResult tick(std::int64_t now_ms, double elapsed_seconds) {
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds <= 0.0) {
      throw std::invalid_argument("elapsed time must be positive");
    }
    if (now_ms < 0 || (has_command_ && now_ms < last_command_ms_)) {
      throw std::invalid_argument("control clock moved backward");
    }

    MotorTickResult result;
    result.requested_linear_x = requested_linear_x_;
    result.requested_angular_z = requested_angular_z_;
    if (!has_command_) {
      result.applied = ramp_.emergencyStop();
      return result;
    }

    const std::int64_t age = now_ms - last_command_ms_;
    result.command_age_ms = static_cast<std::uint32_t>(std::min<std::int64_t>(
        age, std::numeric_limits<std::uint32_t>::max()));
    if (age >= config_.watchdog_ms) {
      result.applied = ramp_.emergencyStop();
      return result;
    }

    double left = requested_linear_x_ - requested_angular_z_;
    double right = requested_linear_x_ + requested_angular_z_;
    const double divisor =
        std::max({1.0, std::fabs(left), std::fabs(right)});
    left = left / divisor * config_.maximum_command;
    right = right / divisor * config_.maximum_command;
    if (config_.invert_left) {
      left = -left;
    }
    if (config_.invert_right) {
      right = -right;
    }

    result.applied = ramp_.update({left, right}, elapsed_seconds);
    result.watchdog_stopped = false;
    return result;
  }

  WheelCommand emergencyStop() { return ramp_.emergencyStop(); }

 private:
  MotorControlConfig config_;
  RampState ramp_;
  bool has_command_{};
  std::int64_t last_command_ms_{};
  double requested_linear_x_{};
  double requested_angular_z_{};
};
