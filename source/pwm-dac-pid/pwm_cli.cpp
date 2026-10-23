// New reconstruction added during portfolio preparation; original host CLI was not recovered.
// POSIX macOS/Linux serial client. Does not flash firmware.
#include "protocol.h"
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
static volatile std::sig_atomic_t running = 1;
static void stop(int) { running = 0; }
static bool send_all(int fd, const std::string& message) {
    size_t sent = 0;
    while (sent < message.size()) {
        ssize_t n = write(fd, message.data() + sent, message.size() - sent);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}
int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: pwm_cli PORT 'READ|MONITOR|STOP' or PORT 'SET|AUTO|KP|KI|KD' INTEGER\n";
        return 2;
    }
    std::string text = argv[2];
    if (argc == 4) text += " " + std::string(argv[3]);
    command cmd;
    if (!parse_command(text.c_str(), &cmd)) {
        std::cerr << "Invalid command or argument\n"; return 2;
    }
    int fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0) { std::perror("open serial port"); return 1; }
    termios original{}, settings{};
    if (tcgetattr(fd, &original) < 0) { std::perror("tcgetattr"); close(fd); return 1; }
    settings = original;
    cfmakeraw(&settings);
    cfsetispeed(&settings, B9600); cfsetospeed(&settings, B9600);
    settings.c_cflag |= CLOCAL | CREAD;
    settings.c_cflag &= ~CSTOPB;
    settings.c_cc[VMIN] = 0; settings.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &settings) < 0) { std::perror("tcsetattr"); close(fd); return 1; }
    std::signal(SIGINT, stop); std::signal(SIGTERM, stop);
    bool okay = send_all(fd, text + "\r"), received = false;
    while (okay && running) {
        pollfd port{fd, POLLIN, 0};
        int ready = poll(&port, 1, cmd.kind == CMD_MONITOR ? 500 : (received ? 200 : 2000));
        if (ready < 0 && errno == EINTR) continue;
        if (ready < 0) { std::perror("poll"); okay = false; break; }
        if (!ready) {
            if (cmd.kind == CMD_MONITOR) continue;
            if (!received) { std::cerr << "No response from board\n"; okay = false; }
            break;
        }
        if (port.revents & (POLLERR | POLLNVAL | POLLHUP)) { okay = false; break; }
        char buffer[256];
        ssize_t n = read(fd, buffer, sizeof buffer);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { okay = false; break; }
        std::cout.write(buffer, n); std::cout.flush(); received = true;
    }
    if (cmd.kind == CMD_MONITOR) send_all(fd, "STOP\r");
    tcsetattr(fd, TCSANOW, &original); close(fd);
    return okay ? 0 : 1;
}
