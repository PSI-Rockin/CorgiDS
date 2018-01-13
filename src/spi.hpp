/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef spi_hpp
#define spi_hpp
#include "firmware.hpp"
#include "touchscreen.hpp"

struct REG_SPICNT
{
    int bandwidth;
    bool busy;
    int device;
    bool transfer_16bit; //Note: Transferring 16 bits is bugged! Only places the second byte in SPIDATA
    bool chipselect_hold;
    bool IRQ_after_transfer;
    bool enabled;
};

class Emulator;

class SPI_Bus
{
    private:
        Emulator* e;
        Firmware firmware;
        TouchScreen touchscreen;
        REG_SPICNT SPICNT;
    
        uint8_t output;
    public:
        SPI_Bus(Emulator* e);
        int init(std::string firmware_path);
        void init(uint8_t* firmware);
        void direct_boot();

        void touchscreen_press(int x, int y);
    
        uint8_t read_SPIDATA();
        void write_SPIDATA(uint8_t data);
    
        uint16_t get_SPICNT();
        void set_SPICNT(uint16_t value);
};

#endif /* spi_hpp */
