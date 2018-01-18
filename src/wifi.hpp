/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef WIFI_HPP
#define WIFI_HPP
#include <cstdint>

//This will just be a stub for now
//Maybe wifi emulation in the future? who knows
class WiFi
{
    private:
        uint16_t W_POWER_US; //0x036
        uint16_t W_BB_WRITE; //0x15A
        uint16_t W_BB_READ; //0x15C
        uint16_t W_BB_MODE; //0x160
        uint16_t W_BB_POWER; //0x168
        uint16_t W_RF_CNT; //0x184

        uint8_t RAM[0x2000];
        uint16_t IO_regs[0x1000];
        uint8_t BB_regs[0x100];

        void BB_READ(int index);
        void BB_WRITE(int index);
    public:
        void power_on();

        uint16_t read(uint32_t address);
        void write(uint32_t address, uint16_t value);
        uint16_t get_W_RANDOM();
        void set_W_POWER_US(uint16_t value);
        void set_W_POWERFORCE(uint16_t value);
        void set_W_BB_CNT(uint16_t value);
        void set_W_BB_WRITE(uint16_t value);
        void set_W_BB_MODE(uint16_t value);
        void set_W_BB_POWER(uint16_t value);
        void set_W_RF_CNT(uint16_t value);
        bool get_W_RF_BUSY();
        uint16_t get_W_BB_READ();
        bool get_W_BB_BUSY();
};

#endif // WIFI_HPP
