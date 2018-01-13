/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdlib>
#include "emulator.hpp"
#include "spi.hpp"

SPI_Bus::SPI_Bus(Emulator* e) : e(e), firmware(e) {}

int SPI_Bus::init(std::string firmware_path)
{
    if (firmware.load_firmware(firmware_path))
    {
        printf("\nFailed to load firmware");
        return 1;
    }
    SPICNT.busy = false;
    SPICNT.enabled = false;
    touchscreen.power_on();
    return 0;
}

void SPI_Bus::init(uint8_t *firmware)
{
    //this->firmware.load(firmware);
    SPICNT.busy = false;
    SPICNT.enabled = false;
    touchscreen.power_on();
}

void SPI_Bus::direct_boot()
{
    firmware.direct_boot();
}

void SPI_Bus::touchscreen_press(int x, int y)
{
    touchscreen.press_event(x, y);
}

uint8_t SPI_Bus::read_SPIDATA()
{
    if (!SPICNT.enabled)
        return 0;
    return output;
}

void SPI_Bus::write_SPIDATA(uint8_t data)
{
    if (SPICNT.enabled)
    {
        SPICNT.busy = false;
        
        //TODO: proper synchronization
        switch (SPICNT.device)
        {
            case 0: //Power management
                output = 0;
                break;
            case 1: //Firmware
                output = firmware.transfer_data(data);
                if (!SPICNT.chipselect_hold)
                    firmware.deselect();
                break;
            case 2: //Touchscreen
                output = touchscreen.transfer_data(data);
                break;
            default:
                output = 0;
        }
        
        if (SPICNT.IRQ_after_transfer)
            e->request_interrupt7(INTERRUPT::SPI);
    }
}

uint16_t SPI_Bus::get_SPICNT()
{
    uint16_t reg = 0;
    reg |= SPICNT.bandwidth;
    reg |= SPICNT.busy << 7;
    reg |= SPICNT.device << 8;
    reg |= SPICNT.transfer_16bit << 10;
    reg |= SPICNT.chipselect_hold << 11;
    reg |= SPICNT.IRQ_after_transfer << 14;
    reg |= SPICNT.enabled << 15;
    return reg;
}

void SPI_Bus::set_SPICNT(uint16_t value)
{
    SPICNT.bandwidth = value & 0x3;
    SPICNT.device = (value >> 8) & 0x3;
    SPICNT.transfer_16bit = value & (1 << 10);
    SPICNT.chipselect_hold = value & (1 << 11);
    SPICNT.IRQ_after_transfer = value & (1 << 14);
    SPICNT.enabled = value & (1 << 15);
}
