#include "commands.h"


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

        outCommands[byteI] = outByte;
    }

    // set the last bit of the last byte in the command to 0
    outCommands[byteCount - 1] &= ((0xFFFF) - 1);
    
}

void combineCommands(int inCommandCount,
                     uint16_t inputCommands[inCommandCount],
                     uint32_t combined[COMBINED_LEN(inCommandCount)])
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

// writes the combined output commands for the OSR, and writes the expected
// number of bytes from the controller in response
void prepareCommand(Command command,
                    uint32_t outputCommands[COMBINED_LEN(command.bytesLength)])
{
    // encode the commands for the SM to read then output
    uint16_t tempCommands[command.bytesLength];
    encodeCommands(command.bytesLength, command.arguments, tempCommands);

    combineCommands(command.bytesLength, tempCommands, outputCommands);
}

void zeroBuffer(Command sending, uint8_t buffer[sending.responseBytesLength])
{
    for(int i = 0; i < sending.responseBytesLength; i++)
        buffer[i] = 0x00;
}


void sendCommand(Command command,
                 uint8_t outBuffer[command.responseBytesLength],
                 AdapterInfo adInf)
{
    zeroBuffer(command, outBuffer);
    
    int combinedSendLen = COMBINED_LEN(command.bytesLength);
    uint32_t outputCommands[combinedSendLen];
    
    // concat commands into 32 bit blocks
    prepareCommand(command, outputCommands);
    
    
    // send command
    printf("Sending command\n");
    
    for(int i = 0; i < combinedSendLen; i++)
    {
        pio_sm_put_blocking(adInf.pio, 0, outputCommands[i]);
        // printf("Sent Data %d: %08x\n", i, outputCommands[i]);
    }

    // sm will now be in "input mode"
    // so pull info
    dma_channel_set_transfer_count(adInf.dmaChannel,
                                   command.responseBytesLength,
                                   false);
    // write to method output buffer
    // trigger start of DMA channel
    dma_channel_set_write_addr(adInf.dmaChannel, outBuffer, true);

    for(int i = 0; i < command.responseBytesLength; i++)
    {
        printf("Read Data %d: %02x\n", i, outBuffer[i]);
    }
    
    // return to outmode
    pio_sm_set_enabled(adInf.pio, 0, false);
    pio_sm_init(adInf.pio,
                0,
                adInf.pioDefaultOffset + adInf.pioOutmodeOffset,
                &adInf.pioConfig);
    pio_sm_set_enabled(adInf.pio, 0, true);

    dma_channel_abort(adInf.dmaChannel);
}

int controllerConnected(AdapterInfo adInf)
{
    Command id = ID;
    uint8_t outBuffer[id.responseBytesLength];
    sendCommand(id, outBuffer, adInf);

    for(int i = 0; i < 3; i++)
        if(outBuffer[i] != 0x00) return 1;

    return 0;
}

