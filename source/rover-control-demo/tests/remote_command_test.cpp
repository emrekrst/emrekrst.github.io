#include "remote_mvp/remote_command.hpp"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

bool rejects(const std::function<void()>& operation) {
  try {
    operation();
    return false;
  } catch (const std::invalid_argument&) {
    return true;
  }
}

}  // namespace

int main() {
  using remote_mvp::RemoteCommand;
  using remote_mvp::RemoteSequenceGate;
  using remote_mvp::encodeRemoteCommand;
  using remote_mvp::parseRemoteCommand;

  const RemoteCommand input{44, 7, 0.5, -0.25};
  const auto encoded = encodeRemoteCommand(input);
  expect(encoded.rfind("ROVER1 ", 0) == 0, "exact ROVER1 prefix");
  expect(!encoded.empty() && encoded.back() == '\n',
         "encoded command ends with one newline");
  const auto output = parseRemoteCommand(encoded);
  expect(output.session_id == 44 && output.sequence == 7,
         "round-trip identity");
  expect(output.linear_x == input.linear_x && output.angular_z == input.angular_z,
         "round-trip command values");
  expect(encodeRemoteCommand({1, 2, 0.1, -0.2}) ==
             "ROVER1 1 2 0.10000000000000001 -0.20000000000000001\n",
         "classic-locale 17-digit encoding");

  const RemoteCommand boundary{0, std::numeric_limits<std::uint64_t>::max(),
                               -1.0, 1.0};
  const auto boundary_output = parseRemoteCommand(encodeRemoteCommand(boundary));
  expect(boundary_output.session_id == boundary.session_id &&
             boundary_output.sequence == boundary.sequence &&
             boundary_output.linear_x == boundary.linear_x &&
             boundary_output.angular_z == boundary.angular_z,
         "negative and boundary values round-trip");

  expect(rejects([] { parseRemoteCommand(std::string(129, 'x')); }),
         "payloads over 128 bytes are rejected");
  expect(rejects([] { parseRemoteCommand("ROVER2 1 2 0 0\n"); }),
         "wrong magic is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0\n"); }),
         "missing field is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0 0 9\n"); }),
         "extra field is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 nan 0\n"); }),
         "NaN is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0 inf\n"); }),
         "infinity is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 1.0000001 0\n"); }),
         "linear value above range is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0 -1.0000001\n"); }),
         "angular value below range is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0 0\ntrailing"); }),
         "trailing data is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0 0"); }),
         "missing terminating newline is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0 0\n\n"); }),
         "extra newline is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 oops 0\n"); }),
         "malformed numeric value is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 1 2 0x0 0\n"); }),
         "hexadecimal numeric value is rejected");
  expect(rejects([] { parseRemoteCommand("ROVER1 -1 2 0 0\n"); }),
         "negative session is rejected");
  expect(rejects([] {
           encodeRemoteCommand({0, 0, std::numeric_limits<double>::quiet_NaN(), 0});
         }),
         "encoding NaN is rejected");
  expect(rejects([] {
           encodeRemoteCommand({0, 0, 0, std::numeric_limits<double>::infinity()});
         }),
         "encoding infinity is rejected");
  expect(rejects([] { encodeRemoteCommand({0, 0, 1.01, 0}); }),
         "encoding out-of-range value is rejected");

  RemoteSequenceGate gate;
  expect(gate.accept({44, 7, 0, 0}), "first sequence is accepted");
  expect(!gate.accept({44, 7, 0, 0}), "duplicate sequence is rejected");
  expect(!gate.accept({44, 6, 0, 0}), "older sequence is rejected");
  expect(gate.accept({45, 0, 0, 0}), "new session is accepted");
  expect(gate.accept({45, 1, 0, 0}), "newer sequence is accepted");

  if (failures != 0) {
    std::cerr << failures << " assertion(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "remote command protocol tests passed\n";
  return EXIT_SUCCESS;
}
