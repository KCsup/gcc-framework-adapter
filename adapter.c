#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "adapter.pio.h"
#include <stdio.h>
#include "hardware/clocks.h"
#include "commands.h"

#define DATA_PIN 28

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
    sm_config_set_out_shift(&pio_config, false, false, 16);
    // from right to left
    // receive bits from LSB up
    // NOT automatically sending the data to the RX FIFO when filled (32 bits)
    // WORKS. DO NOT TOUCH
    sm_config_set_in_shift(&pio_config, false, true, 8);

    pio_sm_init(pio, 0, offset, &pio_config);
    pio_sm_set_enabled(pio, 0, true);

    while(true)
    {
        const Command sending = ID;
        
        int combinedSendLen = COMBINED_LEN(sending.bytesLength);
        uint32_t outputCommands[combinedSendLen];

        prepareCommand(sending, outputCommands);
        
        
        // send command
        printf("Sending command\n");
        
        // while(true)
        // {
        for(int i = 0; i < combinedSendLen; i++)
        {
            pio_sm_put_blocking(pio, 0, outputCommands[i]);
            // while(true)
            printf("Sent Data %d: %08x\n", i, outputCommands[i]);
        }
        // }

        // sm will now be in "input mode"
        // so pull info

        for(int i = 0; i < sending.responseBytesLength; i++)
        {
            uint32_t readData = pio_sm_get_blocking(pio, 0);

            printf("Read Data %d: %08x\n", i, readData);
        }
        pio_sm_set_enabled(pio, 0, false);
        pio_sm_init(pio, 0, offset + adapter_offset_out_init, &pio_config);
        pio_sm_set_enabled(pio, 0, true);
    }
    
    return 0;
}
