/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdlib>
#include <cstdio>
#include "wifi.hpp"

//All of this is very much TODO

#define WIFI_REG(addr) IO_regs[addr >> 1]

void WiFi::power_on()
{
    for (int i = 0; i < 0x1000; i++)
        IO_regs[i] = 0;
}

uint16_t WiFi::read(uint32_t address)
{
    address &= 0x7FFF;
    if (address >= 0x4000 && address < 0x6000)
        return *(uint16_t*)&RAM[address & 0x1FFF];
    switch (address)
    {
        case 0x0044:
            return get_W_RANDOM();
        case 0x015C:
            return get_W_BB_READ();
        case 0x015E:
            return get_W_BB_BUSY();
        case 0x0180:
            return get_W_RF_BUSY();
    }
    printf("\n[WiFi] Unrecognized read16 at $%08X", address);
    if (address < 0x2000)
        return IO_regs[address >> 1];
    return 0;
}

void WiFi::write(uint32_t address, uint16_t value)
{
    address &= 0x7FFF;
    if (address >= 0x4000 && address < 0x6000)
    {
        *(uint16_t*)&RAM[address & 0x1FFF] = value;
        return;
    }
    switch (address)
    {
        case 0x0036:
            set_W_POWER_US(value);
            return;
        case 0x0040:
            set_W_POWERFORCE(value);
            return;
        case 0x0158:
            set_W_BB_CNT(value);
            return;
        case 0x015A:
            set_W_BB_WRITE(value);
            return;
        case 0x0160:
            set_W_BB_MODE(value);
            return;
        case 0x0168:
            set_W_BB_POWER(value);
            return;
        case 0x0184:
            set_W_RF_CNT(value);
            return;
    }
    printf("\n[WiFi] Unrecognized write16 of $%04X at $%08X", value, address);
    if (address < 0x2000)
        IO_regs[address >> 1] = value;
}

uint16_t WiFi::get_W_RANDOM()
{
    //TODO: return real algorithm
    return rand() & 0x3FF;
}

void WiFi::set_W_POWER_US(uint16_t value)
{
    W_POWER_US = value;
}

void WiFi::set_W_POWERFORCE(uint16_t value)
{
    //TODO: proper handling of W_POWERSTATE
    if (value & 0x8001)
    {
        //Taken from GBAtek:
        /*[4808034h]=0002h ;W_INTERNAL
        [480803Ch]=02xxh ;W_POWERSTATE
        [48080B0h]=0000h ;W_TXREQ_READ
        [480819Ch]=0046h ;W_RF_PINS
        [4808214h]=0009h ;W_RF_STATUS (idle);*/
        WIFI_REG(0x0034) = 0x2;
        WIFI_REG(0x003C) = (WIFI_REG(0x003C) & 0xFF) | 0x0200;
        WIFI_REG(0x00B0) = 0;
        WIFI_REG(0x019C) = 0x46;
        WIFI_REG(0x0214) = 0x9;
    }
}

void WiFi::BB_READ(int index)
{
    switch (index)
    {
        case 0x00:
            W_BB_READ = 0x6D;
            break;
        default:
            W_BB_READ = BB_regs[index];
    }
}

void WiFi::BB_WRITE(int index)
{
    BB_regs[index] = W_BB_WRITE;
}

void WiFi::set_W_BB_CNT(uint16_t value)
{
    int index = value & 0xFF;
    int direction = (value >> 12);
    if (direction == 5)
        BB_WRITE(index);
    else if (direction == 6)
        BB_READ(index);
}

void WiFi::set_W_BB_MODE(uint16_t value)
{
    W_BB_MODE = value;
}

void WiFi::set_W_BB_WRITE(uint16_t value)
{
    W_BB_WRITE = value;
}

void WiFi::set_W_BB_POWER(uint16_t value)
{
    W_BB_POWER = value;
}

void WiFi::set_W_RF_CNT(uint16_t value)
{
    W_RF_CNT = value;
}

bool WiFi::get_W_RF_BUSY()
{
    return false;
}

uint16_t WiFi::get_W_BB_READ()
{
    return W_BB_READ;
}

bool WiFi::get_W_BB_BUSY()
{
    return false;
}
