/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "wifi.hpp"

//All of this is very much TODO
void WiFi::set_W_POWER_US(uint16_t value)
{
    W_POWER_US = value;
}

void WiFi::BB_READ(int index)
{
    switch (index)
    {
        case 0:
            W_BB_READ = 0x6D;
            break;
        default:
            W_BB_READ = 0;
            break;
    }
}

void WiFi::BB_WRITE(int index)
{
    //TODO
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
