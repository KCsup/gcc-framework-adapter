#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include <math.h>

typedef struct {
    int bytesLength;
    int responseBytesLength;
    uint8_t arguments[3]; // of length byte length
} Command;

#define ID ((Command) { 1, 3, { 0x00, 0, 0 } })
#define STATUS ((Command) { 3, 8, { 0x40, 0x00, 0x03 } })
#define ORIGIN ((Command) { 1, 10, { 0x41, 0, 0 } })

#define MAX_COMMAND_RESPONSE_LEN 10

#define COMBINED_LEN(args) ((int) ceil((double) args / 2))

void prepareCommand(Command command,
                    uint32_t outputCommands[COMBINED_LEN(command.bytesLength)]);

#endif
