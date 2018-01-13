/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include "touchscreen.hpp"

TouchScreen::TouchScreen()
{
    data_pos = 0;
    control_byte = 0;
    output_coords = 0;

    press_x = 0;
    press_y = 0xFFF;
}

void TouchScreen::power_on()
{
    data_pos = 0;
    control_byte = 0;
    output_coords = 0;

    press_x = 0;
    press_y = 0xFFF;
}

void TouchScreen::press_event(int x, int y)
{
    press_x = x;
    press_y = y;
    if (y == 0xFFF)
        return;
    press_x <<= 4;
    press_y <<= 4;

    //printf("\nTouchscreen: ($%04X, $%04X)", press_x, press_y);
}

uint8_t TouchScreen::transfer_data(uint8_t input)
{
    uint8_t data;
    if (data_pos == 0)
        data = (output_coords >> 5) & 0xFF;
    else if (data_pos == 1)
        data = (output_coords << 3) & 0xFF;
    else
        data = 0;
    if (input & (1 << 7))
    {
        //Set control byte
        control_byte = input;
        int channel = (control_byte >> 4) & 0x7;
        data_pos = 0;

        switch (channel)
        {
            case 1:
                //touch Y
                output_coords = press_y;
                break;
            case 5:
                //touch X
                output_coords = press_x;
                break;
            case 6:
                output_coords = 0x800;
                break;
            default:
                output_coords = 0xFFF;
                break;
        }

        //Conversion mode change
        if (control_byte & 0x8)
        {
            output_coords &= 0x0FF0;
        }
    }
    else
        data_pos++;
    return data;
}

void TouchScreen::deselect()
{
    data_pos = 0;
}
