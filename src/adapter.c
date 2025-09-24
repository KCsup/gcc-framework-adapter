#include "adapter.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "adapter.pio.h"
#include "hardware/clocks.h"
#include "commands.h"
#include "hardware/dma.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include <stdio.h>

// TODO: LED Debug
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 25
// ---

#define DATA_PIN 17

volatile bool hidReportPending = false;

int main()
{
    // TODO: LED:
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    set_led(0);
    // ---
    
    // USB
    board_init();
    tusb_init();

    if(board_init_after_tusb)
    {
        board_init_after_tusb();
    }

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
    sm_config_set_out_shift(&pio_config, false, false, 32);
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
        0,
        false
    );


    hidReportPending = true;

    const AdapterInfo adInf = {
        pio,
        pio_config,
        offset,
        adapter_offset_out_init,
        dmaChannel
    };

    int originSent = 0;
    uint8_t dolphinOrigin[6] = { 0x00 };
    
    while(true)
    {
        tud_task();
        

        if(tud_hid_ready() && hidReportPending)
        {
            uint8_t controllerReport[RESPONSE_LEN] = DEFAULT_RESPONSE;

            if(!sendCommand(ID, NULL, adInf))
            {
                originSent = 0;
                // controllerReport[0] = 0xAA;
            }
            else
            {
                sleep_us(50);
                // controller is connected
                // check for if origin was sent
                if(!originSent)
                {
                    Command origin = ORIGIN;
                    uint8_t originResponse[origin.responseBytesLength];
                    sendCommand(origin, originResponse, adInf);

                    // start writing origin data at the 1st index
                    dolphinFormatOrigin(originResponse, dolphinOrigin);

                    // now, an array for the origin data is available
                   
                    originSent = 1;

                    sleep_us(50);
                }

                // const Command sending = ORIGIN;

                Command sending = STATUS;
                uint8_t receiveBuffer[sending.responseBytesLength];
                
                // int statusResponse = sendCommand(sending,
                //                                  receiveBuffer,
                //                                  adInf);

                // if(!statusResponse) continue;
                int statusResponse = sendCommand(sending,
                                                 receiveBuffer,
                                                 adInf);

                if(statusResponse)
                {
                    
                    controllerReport[1] |= 0x10;

                    dolphinFormatStatus(receiveBuffer);

                    applyDolphinOrigin(receiveBuffer, dolphinOrigin);
                    
                    for(int i = 0; i <= sending.responseBytesLength; i++)
                    {
                        // info for first controller starts at byte 2 (index 1)
                        controllerReport[i + 2] = receiveBuffer[i];
                    }
                }
            }

            tud_hid_report(0, controllerReport, sizeof(controllerReport));
            
            hidReportPending = false;
        }

    }

    
    dma_channel_unclaim(dmaChannel);
    return 0;
}


void tud_hid_report_complete_cb(uint8_t instance,
                                uint8_t const* report,
                                uint16_t len)
{
    hidReportPending = true;
}

// device to host
uint16_t tud_hid_get_report_cb(uint8_t itf,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t* buffer,
                               uint16_t reqlen)
{
    return 0;
}

// host to device
void tud_hid_set_report_cb(uint8_t itf,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer,
                           uint16_t bufsize)
{
    // empty
    // if(report_type == HID_REPORT_TYPE_OUTPUT)
    // {
        if(buffer[0] == 0x13)
            set_led(1);
    // }

    // set_led(0);
}

void set_led(int state)
{
    gpio_put(LED_PIN, state);
}
