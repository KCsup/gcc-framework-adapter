#ifndef ADAPTER_H
#define ADAPTER_H

#include <stdint.h>
#include <math.h>

typedef struct {
    int bytesLength;
    int responseBytesLength;
    uint8_t arguments[3]; // of length byte length
} Command;

#define ID ((Command) { 1, 3, { 0x00, 0, 0 } })
#define STATUS ((Command) { 1, 1, { 0x40, 0x00, 0x03 } })
#define ORIGIN ((Command) { 1, 5, { 0x41, 0, 0 } })

#define COMBINED_LEN(args) ((int) ceil((double) args / 2))

#endif
