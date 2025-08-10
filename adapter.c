#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "adapter.pio.h"
#include <stdint.h>
#include <stdio.h>
#include "hardware/clocks.h"

#define DATA_PIN 28

void encodeCommands(int byteCount,
                   uint8_t commands[byteCount],
                   uint16_t outCommands[byteCount])
{
    for(int byteI = 0; byteI < byteCount; byteI++)
    {
        const uint16_t CURRENT_BYTE = (uint16_t) commands[byteI];
        uint16_t outByte = 0x0000;

        // MSB to LSB
        for(int bitI = 7; bitI >= 0; bitI--)
            // isolate the data bit, move it to the EVEN position toward the
            // MSB, then insert a one next to the EVEN position of the shifted
            // bit (in the ODD position)
            outByte |= ((CURRENT_BYTE & (1 << bitI)) << (bitI + 1))
                | (1 << (bitI * 2));

        // set the last bit in the command to 0
        outByte &= ((0xFFFF) - 1);

        outCommands[byteI] = outByte;
    }
    
}

void combineCommands(int inCommandCount,
                     uint16_t inputCommands[inCommandCount],
                     uint32_t combined[])
{
    // hex value visualization:
    // 0xAA to 16 bit -> 0x00AA
    // 0xAA to 32 bit -> 0x000000AA
    // this function takes any amount of 16 bit ints from an array, then blocks
    // them into 32 bit ints for a new array
    // ex. [ 0x00AA, 0x00BB, 0x00CC]
    // returns [ 0x00AA00BB, 0x00CC0000 ]
    int currentAppend = 0;
    for(int i = 0; i < inCommandCount; i += 2)
    {
        int j = i + 1;

        uint32_t currentCommandShift = ((uint32_t) inputCommands[i]) << 16;
        if(j >= inCommandCount) // on a loner command
            combined[currentAppend] = currentCommandShift;
        else
            combined[currentAppend] = currentCommandShift | ((uint32_t) inputCommands[j]);

        currentAppend++;
    }
}

int main()
{
    stdio_init_all();

    // set sys clock to 125,000 KHz (125 MHz)
    set_sys_clock_khz(125000, true);

    gpio_init(DATA_PIN);
    gpio_set_dir(DATA_PIN, GPIO_IN); // since pullup is only accessible for
                                     // input pins
    gpio_pull_up(DATA_PIN);

    sleep_us(100);

    PIO pio = pio0;
    pio_gpio_init(pio, DATA_PIN);
    int offset = pio_add_program(pio, &adapter_program); // from pio header

    pio_sm_config pio_config = adapter_program_get_default_config(offset);
    sm_config_set_in_pins(&pio_config, DATA_PIN);
    sm_config_set_out_pins(&pio_config, DATA_PIN, 1);
    sm_config_set_set_pins(&pio_config, DATA_PIN, 1);
    sm_config_set_clkdiv(&pio_config, 5); // set SM clock to 25 MHz

    // from left to right
    // send bits from the MSB down
    sm_config_set_out_shift(&pio_config, false, true, 16);
    // from right to left
    // receive bits from LSB up
    // NOT automatically sending the data to the RX FIFO when filled (32 bits)
    // WORKS. DO NOT TOUCH
    sm_config_set_in_shift(&pio_config, false, true, 8);

    pio_sm_init(pio, 0, offset, &pio_config);
    pio_sm_set_enabled(pio, 0, true);

    while(true)
    {
        const int COMMAND_SIZE = 1;
        uint8_t command[1] = { 0x00 };
        
        uint16_t encoded[COMMAND_SIZE];
        encodeCommands(COMMAND_SIZE, command, encoded);
        
        uint32_t readyCommands[1];
        combineCommands(COMMAND_SIZE, encoded, readyCommands);

        // send command
        printf("Sending command\n");
        
        for(int i = 0; i < 1; i++)
        {
            pio_sm_put_blocking(pio, 0, readyCommands[i]);
            // while(true)
            printf("Sent Data: %08x\n", readyCommands[i]);
        }

        // sm will now be in "input mode"
        // so pull info

        const int IN_SIZE = 3;
        for(int i = 0; i < IN_SIZE; i++)
        {
            uint8_t readData = pio_sm_get_blocking(pio, 0);

            printf("Read Data: %02x\n", readData);
        }
        pio_sm_set_enabled(pio, 0, false);
        pio_sm_init(pio, 0, offset + adapter_offset_out_init, &pio_config);
        pio_sm_set_enabled(pio, 0, true);
    }
    
    return 0;
}
