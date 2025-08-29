#include <stdint.h>
#include <stdlib.h>
#include "tusb.h"

const uint8_t deviceDescriptor[] =
{
    0x12,
    0x01,
    // 0x0200
    0x00,
    0x02,
    
    0x00,
    0x00,
    0x00,
    0x40,
    // 0x057E
    0x7E,
    0x05,

    // 0x0337
    0x37,
    0x03,

    // 0x0100
    0x00,
    0x01,

    0x01,
    0x02,
    0x03,
    0x01
};

const uint8_t configDescriptor[] =
{
    // config
    0x09,
    0x02,
    // 0x0029
    0x29,
    0x00,

    0x01,
    0x01,
    0x00,
    0xE0,
    0xFA, // 500 mA (?)

    // interface
    0x09,
    0x04,
    0x00,
    0x00,
    0x02,
    0x03,
    0x00,
    0x00,
    0x00,
    
    // HID
    0x09,
    0x21,
    // 0x0110
    0x10,
    0x01,

    0x00,
    0x01,
    0x22,
    // 0x00D6
    0xD6,
    0x00,

    // endpoint 1 (in)
    0x07,
    0x05,
    0x81,
    0x03,
    // 0x0025
    0x25,
    0x00,
    0x01,

    // endpoint 2 (out)
    0x07,
    0x05,
    0x02,
    0x03,
    // 0x0005
    0x05,
    0x00,
    0x01
};

const char* stringDescriptorArr[] =
{
    (const char[]) { 0x09, 0x04 }, // 0x0409
    "KCsup Deamons", // manufacturer
    "GCC Framework", // product
    "1" //serial
};

const uint8_t hidDescriptor[] =
{
    0x09,
    0x21,
    // 0x0110
    0x10,
    0x01,

    0x00,
    0x01,
    0x22,
    // 0x00D6
    0xD6,
    0x00
};

const uint8_t* tud_descriptor_device_cb(void)
{
    return deviceDescriptor;
}

const uint8_t* tud_hid_descriptor_report_cb(uint8_t instance)
{
    return hidDescriptor;
}

const uint8_t* tud_descriptor_configuration_cb(uint8_t index)
{
    return configDescriptor;
}

// local global
const size_t MAX_STR_LEN = 32;
static uint16_t stringDescriptor[33];

const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    size_t stringLen;

    if(index == 0)
    {
        memcpy(&stringDescriptor[1], stringDescriptorArr[0], 2);
        stringLen = 1;
    }
    else
    {
        const char* string = stringDescriptorArr[index];

        stringLen = strlen(string);

        if(stringLen > MAX_STR_LEN)
            stringLen = MAX_STR_LEN;

        // convert ASCII string to UTF-16
        for(size_t i = 0; i < stringLen; i++)
        {
            stringDescriptor[i + 1] = string[i];
        }
    }

    // first byte is length (including header of command), second is str type
    // (0x03)
    // also, order is flipped (because USB byte groups)
    stringDescriptor[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | ((2 * stringLen) + 2));

    return stringDescriptor;
}

