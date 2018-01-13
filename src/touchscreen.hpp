/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef TOUCHSCREEN_HPP
#define TOUCHSCREEN_HPP
#include <cstdint>

class TouchScreen
{
    private:
        uint8_t control_byte;
        uint16_t output_coords;
        int data_pos;

        uint16_t press_x, press_y;
    public:
        TouchScreen();
        void power_on();

        void press_event(int x, int y);

        uint8_t transfer_data(uint8_t input);
        void deselect();
};

#endif // TOUCHSCREEN_HPP
