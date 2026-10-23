#ifndef ROVER_MOVE_H
#define ROVER_MOVE_H

#include <cstdlib>
#include <cstring>
#include <cmath>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"


// ============================================================
//  Drive FSM states
//   BOOT          – one-shot hardware init
//   WAIT_LINK     – motors off, waiting for the first valid letter
//   ACTIVE_DRIVE  – letter → PWM every cycle
//   FAILSAFE_STOP – timeout / corrupt stream → motors off until fresh letter
// ============================================================
enum class DriveState {
    BOOT,
    WAIT_LINK,
    ACTIVE_DRIVE,
    FAILSAFE_STOP
};

// ============================================================
//  Runtime context  (all mutable state in one struct)
// ============================================================
struct MachineContext {
    // Motor pins: M1 → GP1  (DIR) / GP0  (PWM)
    //             M2 → GP10 (DIR) / GP11 (PWM)
    // UART0 RX → GP17 (Pi GPIO14 TX), UART0 TX → GP16
    const uint M1_DIR_PIN = 1;
    const uint M1_PWM_PIN = 0;
    const uint M2_DIR_PIN = 10;
    const uint M2_PWM_PIN = 11;

    // PWM hardware
    const int  MAX_PWM  = 255;
    uint16_t   pwm_wrap = 0;

    // UART receive buffer (newline-delimited protocol)
    char     serial_buf[64] = {};
    int      buf_index      = 0;
    bool     discarding_line = false;
    bool     has_received = false;
    uint32_t last_rx_ms     = 0;   // ms timestamp of last valid letter

    // Current drive values
    char last_letter = '?';
    int  curr_l      = 0;          // slew-limited left  PWM output
    int  curr_r      = 0;          // slew-limited right PWM output

    // Safety timeout: enter FAILSAFE_STOP if no valid letter for this long
    const uint32_t RX_TIMEOUT_MS = 300;

};

// ============================================================
//  Motor primitives
// ============================================================
inline void setMotorSpeeds(MachineContext& ctx, int pwm1, int pwm2) {
    // Motor 1
    gpio_put(ctx.M1_DIR_PIN, pwm1 >= 0 ? 1 : 0);
    int sp1 = std::abs(pwm1);
    if (sp1 > ctx.MAX_PWM) sp1 = ctx.MAX_PWM;
    pwm_set_gpio_level(ctx.M1_PWM_PIN,
                       (uint16_t)((sp1 * ctx.pwm_wrap) / ctx.MAX_PWM));

    // Motor 2
    gpio_put(ctx.M2_DIR_PIN, pwm2 >= 0 ? 1 : 0);
    int sp2 = std::abs(pwm2);
    if (sp2 > ctx.MAX_PWM) sp2 = ctx.MAX_PWM;
    pwm_set_gpio_level(ctx.M2_PWM_PIN,
                       (uint16_t)((sp2 * ctx.pwm_wrap) / ctx.MAX_PWM));
}

inline void stopMotors(MachineContext& ctx) {
    setMotorSpeeds(ctx, 0, 0);
    ctx.curr_l = 0;
    ctx.curr_r = 0;
}

// ============================================================
//  Hardware init  (called once in BOOT state)
// ============================================================
inline void initHardware(MachineContext& ctx) {
    stdio_init_all();

    // PWM at ~20 kHz
    uint32_t sys_clk = clock_get_hz(clk_sys);
    ctx.pwm_wrap     = (uint16_t)(sys_clk / 20000);

    // Motor 1: DIR = GP1, PWM = GP0
    gpio_init(ctx.M1_DIR_PIN);
    gpio_set_dir(ctx.M1_DIR_PIN, GPIO_OUT);
    gpio_set_function(ctx.M1_PWM_PIN, GPIO_FUNC_PWM);
    uint slice1 = pwm_gpio_to_slice_num(ctx.M1_PWM_PIN);
    pwm_set_wrap(slice1, ctx.pwm_wrap);
    pwm_set_enabled(slice1, true);

    // Motor 2: DIR = GP10, PWM = GP11
    gpio_init(ctx.M2_DIR_PIN);
    gpio_set_dir(ctx.M2_DIR_PIN, GPIO_OUT);
    gpio_set_function(ctx.M2_PWM_PIN, GPIO_FUNC_PWM);
    uint slice2 = pwm_gpio_to_slice_num(ctx.M2_PWM_PIN);
    pwm_set_wrap(slice2, ctx.pwm_wrap);
    pwm_set_enabled(slice2, true);

    stopMotors(ctx);

    // UART0 at 9600 baud: GP16 = TX (Pico → Pi), GP17 = RX (Pi → Pico)
    uart_init(uart0, 9600);
    gpio_set_function(16, GPIO_FUNC_UART);
    gpio_set_function(17, GPIO_FUNC_UART);
}


// ============================================================
//  Letter → motor speed mapping
//
//  Zone index 0-25 maps A=0 … Z=25 (or a=0 … z=25).
//  t in [-1, +1]: negative = left, positive = right.
//
//  Differential drive mixing (outer motor stays at BASE,
//  inner motor scales toward -BASE):
//    left  = BASE – clamp(t, 0, 1) × 2 × BASE
//    right = BASE + clamp(t,-1, 0) × 2 × BASE
//
//  BASE = 120. K-P/k-p form the straight-ahead deadband.
//  Only A/a and Z/z may reverse one wheel; intermediate zones
//  keep the inner motor at a minimum demand of 24.
//  '?' requests zero targets through the same slew limiter.
//
//  Slew-rate limit of 8 PWM units per call reduces jerk.
// ============================================================
inline void driveFromLetter(MachineContext& ctx, char c) {
    int target_l, target_r;

    int zone = -1;
    if      (c >= 'A' && c <= 'Z') zone = c - 'A';
    else if (c >= 'a' && c <= 'z') zone = c - 'a';

    if (zone < 0) {
        // '?' or any unrecognised character → soft stop via slew limiting
        target_l = 0;
        target_r = 0;
    } else {
        // Treat a wider center band as straight ahead so small CV jitter does
        // not keep nudging the rover left and right while it crosses a gate.
        if (zone >= 10 && zone <= 15) {
            zone = 12;
        }

        // Continuous differential drive mixing
        float t = zone == 12 ? 0.0f : (zone - 12.5f) / 12.5f;
        if (t < -1.0f) t = -1.0f;
        if (t >  1.0f) t =  1.0f;

        const float BASE = 120.0f;
        float t_pos = (t > 0.0f) ? t : 0.0f;
        float t_neg = (t < 0.0f) ? t : 0.0f;

        target_l = (int)(BASE - t_pos * 2.0f * BASE);
        target_r = (int)(BASE + t_neg * 2.0f * BASE);

        // Keep intermediate steering letters from turning into violent
        // spin-in-place commands. Only the extreme edge zones may reverse one wheel.
        const int MIN_INNER_PWM = 24;
        if (zone > 0 && zone < 25) {
            if (target_l < MIN_INNER_PWM) target_l = MIN_INNER_PWM;
            if (target_r < MIN_INNER_PWM) target_r = MIN_INNER_PWM;
        }

        // Clamp to motor range
        if (target_l >  ctx.MAX_PWM) target_l =  ctx.MAX_PWM;
        if (target_l < -ctx.MAX_PWM) target_l = -ctx.MAX_PWM;
        if (target_r >  ctx.MAX_PWM) target_r =  ctx.MAX_PWM;
        if (target_r < -ctx.MAX_PWM) target_r = -ctx.MAX_PWM;
    }

    // Slew-rate limit: smaller changes per cycle keep the two-wheel chassis calm.
    const int SLEW = 8;
    int dl = target_l - ctx.curr_l;
    int dr = target_r - ctx.curr_r;
    if (dl >  SLEW) dl =  SLEW;
    if (dl < -SLEW) dl = -SLEW;
    if (dr >  SLEW) dr =  SLEW;
    if (dr < -SLEW) dr = -SLEW;

    ctx.curr_l += dl;
    ctx.curr_r += dr;

    setMotorSpeeds(ctx, ctx.curr_l, ctx.curr_r);
}

// ============================================================
//  Serial line receiver
//
//  Protocol: one ASCII letter (or '?') followed by '\n'.
//  Malformed lines (wrong length, bad char, overlong) are
//  silently discarded.
//
//  Returns true and updates ctx.last_letter / ctx.last_rx_ms
//  when a valid letter is parsed from the stream.
// ============================================================
inline bool updateContextFromSerial(MachineContext& ctx) {
    bool got_letter = false;

    while (uart_is_readable(uart0)) {
        char c = uart_getc(uart0);

        if (c == '\n' || c == '\r') {
            if (ctx.discarding_line) {
                ctx.discarding_line = false;
                ctx.buf_index = 0;
                continue;
            }
            if (ctx.buf_index > 0) {
                ctx.serial_buf[ctx.buf_index] = '\0';
                char letter = ctx.serial_buf[0];

                // Valid: exactly one letter A-Z, a-z, or '?'
                bool valid_char = ((letter >= 'A' && letter <= 'Z') ||
                                   (letter >= 'a' && letter <= 'z') ||
                                   letter == '?');

                if (ctx.buf_index == 1 && valid_char) {
                    ctx.has_received = true;
                    ctx.last_letter = letter;
                    ctx.last_rx_ms  = to_ms_since_boot(get_absolute_time());
                    got_letter      = true;
                }
                // Silently drop malformed lines and reset buffer
                ctx.buf_index = 0;
                memset(ctx.serial_buf, 0, sizeof(ctx.serial_buf));
            }
        } else {
            if (ctx.discarding_line) continue;
            if (ctx.buf_index < 63) {
                ctx.serial_buf[ctx.buf_index++] = c;
            } else {
                // Discard through the delimiter; a suffix must never become a command.
                ctx.discarding_line = true;
                ctx.buf_index = 0;
                memset(ctx.serial_buf, 0, sizeof(ctx.serial_buf));
            }
        }
    }

    return got_letter;
}

// Returns true when the link has timed out (no valid letter within RX_TIMEOUT_MS).
// Safe before the first valid command, including a command received at time zero.
inline bool isLinkTimedOut(const MachineContext& ctx) {
    if (!ctx.has_received) return false;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    return (now - ctx.last_rx_ms) > ctx.RX_TIMEOUT_MS;
}

#endif // ROVER_MOVE_H
