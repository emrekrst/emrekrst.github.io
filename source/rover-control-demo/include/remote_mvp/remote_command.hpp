#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace remote_mvp {

struct RemoteCommand {
  std::uint64_t session_id;
  std::uint64_t sequence;
  double linear_x;
  double angular_z;
};

namespace detail {

inline bool isDecimalFloatingPoint(std::string_view field) {
  std::size_t index = 0;
  if (index < field.size() && (field[index] == '+' || field[index] == '-')) {
    ++index;
  }

  bool integer_digits = false;
  while (index < field.size() && field[index] >= '0' &&
         field[index] <= '9') {
    integer_digits = true;
    ++index;
  }

  bool fraction_digits = false;
  if (index < field.size() && field[index] == '.') {
    ++index;
    while (index < field.size() && field[index] >= '0' &&
           field[index] <= '9') {
      fraction_digits = true;
      ++index;
    }
  }
  if (!integer_digits && !fraction_digits) {
    return false;
  }

  if (index < field.size() && (field[index] == 'e' || field[index] == 'E')) {
    ++index;
    if (index < field.size() && (field[index] == '+' || field[index] == '-')) {
      ++index;
    }
    const auto exponent_start = index;
    while (index < field.size() && field[index] >= '0' &&
           field[index] <= '9') {
      ++index;
    }
    if (index == exponent_start) {
      return false;
    }
  }
  return index == field.size();
}

inline void validateCommandValues(const RemoteCommand& command) {
  if (!std::isfinite(command.linear_x) || !std::isfinite(command.angular_z)) {
    throw std::invalid_argument("remote command values must be finite");
  }
  if (command.linear_x < -1.0 || command.linear_x > 1.0 ||
      command.angular_z < -1.0 || command.angular_z > 1.0) {
    throw std::invalid_argument("remote command values must be in [-1, 1]");
  }
}

inline std::uint64_t parseUnsigned(std::string_view field,
                                   const char* field_name) {
  if (field.empty()) {
    throw std::invalid_argument(std::string("missing ") + field_name);
  }
  std::uint64_t value = 0;
  const auto result = std::from_chars(field.data(), field.data() + field.size(),
                                      value, 10);
  if (result.ec != std::errc{} || result.ptr != field.data() + field.size()) {
    throw std::invalid_argument(std::string("invalid ") + field_name);
  }
  return value;
}

inline double parseDouble(std::string_view field, const char* field_name) {
  if (field.empty() || !isDecimalFloatingPoint(field)) {
    throw std::invalid_argument(std::string("missing ") + field_name);
  }

  std::istringstream stream{std::string(field)};
  stream.imbue(std::locale::classic());
  double value = 0.0;
  stream >> value;
  if (stream.fail() || stream.peek() != std::char_traits<char>::eof() ||
      !std::isfinite(value)) {
    throw std::invalid_argument(std::string("invalid ") + field_name);
  }
  return value;
}

}  // namespace detail

inline std::string encodeRemoteCommand(const RemoteCommand& command) {
  detail::validateCommandValues(command);

  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << "ROVER1 " << command.session_id << ' ' << command.sequence << ' '
         << std::setprecision(17) << command.linear_x << ' ' << command.angular_z
         << '\n';
  const auto payload = stream.str();
  if (payload.size() > 128) {
    throw std::invalid_argument("encoded remote command exceeds 128 bytes");
  }
  return payload;
}

inline RemoteCommand parseRemoteCommand(std::string_view payload) {
  if (payload.size() > 128) {
    throw std::invalid_argument("remote command exceeds 128 bytes");
  }
  if (payload.size() < 2 || payload.back() != '\n') {
    throw std::invalid_argument("remote command must end with one newline");
  }

  const auto body = payload.substr(0, payload.size() - 1);
  std::array<std::string_view, 5> fields{};
  std::size_t start = 0;
  for (std::size_t index = 0; index < fields.size() - 1; ++index) {
    const auto separator = body.find(' ', start);
    if (separator == std::string_view::npos) {
      throw std::invalid_argument("remote command has missing fields");
    }
    fields[index] = body.substr(start, separator - start);
    start = separator + 1;
  }
  fields.back() = body.substr(start);
  if (fields[0] != "ROVER1" || fields[0].empty() || fields[1].empty() ||
      fields[2].empty() || fields[3].empty() || fields[4].empty() ||
      fields[4].find(' ') != std::string_view::npos) {
    throw std::invalid_argument("malformed remote command");
  }

  RemoteCommand command{
      detail::parseUnsigned(fields[1], "session id"),
      detail::parseUnsigned(fields[2], "sequence"),
      detail::parseDouble(fields[3], "linear command"),
      detail::parseDouble(fields[4], "angular command")};
  detail::validateCommandValues(command);
  return command;
}

class RemoteSequenceGate {
 public:
  bool accept(const RemoteCommand& command) {
    if (!has_last_ || command.session_id != session_id_) {
      has_last_ = true;
      session_id_ = command.session_id;
      sequence_ = command.sequence;
      return true;
    }
    if (command.sequence <= sequence_) {
      return false;
    }
    sequence_ = command.sequence;
    return true;
  }

 private:
  bool has_last_ = false;
  std::uint64_t session_id_ = 0;
  std::uint64_t sequence_ = 0;
};

}  // namespace remote_mvp
