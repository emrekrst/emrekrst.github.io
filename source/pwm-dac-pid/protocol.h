#ifndef PWM_PROTOCOL_H
#define PWM_PROTOCOL_H
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {CMD_SET, CMD_AUTO, CMD_READ, CMD_MONITOR, CMD_STOP, CMD_KP, CMD_KI, CMD_KD} command_kind;
typedef struct { command_kind kind; int value; } command;
typedef struct {char text[50]; unsigned length; int overflow;} command_buffer;
int parse_command(const char* text, command* out);
/* 1: complete line, -1: discarded oversized line, 0: incomplete/empty line. */
int command_feed(command_buffer* buffer, char byte);
#ifdef __cplusplus
}
#endif
#endif
