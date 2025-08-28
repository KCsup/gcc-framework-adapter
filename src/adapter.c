#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "adapter.pio.h"
#include <stdio.h>
#include "hardware/clocks.h"
#include "commands.h"
#include "hardware/dma.h"

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

    // DMA conf
    const int dmaChannel = dma_claim_unused_channel(true); // required bool
    dma_channel_config dmaConfig = dma_channel_get_default_config(dmaChannel);

    // configured to send data blocks in bytes
    channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_8);

    // always read from the same address
    channel_config_set_read_increment(&dmaConfig, false);
    // increment the write address (inside the buffer)
    channel_config_set_write_increment(&dmaConfig, true);

    // sets the DMA dreq to the SM's RX FIFO (false in the get_dreq specifies
    // the RX FIFO)
    channel_config_set_dreq(&dmaConfig, pio_get_dreq(pio, 0, false));

    dma_channel_configure(
        dmaChannel,
        &dmaConfig,
        NULL,
        &pio->rxf[0], // address for RX FIFO
        MAX_COMMAND_RESPONSE_LEN,
        false // don't start now
    );

    while(true)
    {
        const Command sending = ORIGIN;

        uint8_t receiveBuffer[sending.responseBytesLength];
        sendCommand(sending,
                    receiveBuffer,
                    pio,
                    pio_config,
                    offset,
                    adapter_offset_out_init,
                    dmaChannel);
        
        sleep_ms(1000);
    }

    
    dma_channel_unclaim(dmaChannel);
    return 0;
}
