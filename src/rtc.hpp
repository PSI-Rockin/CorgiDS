/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef rtc_hpp
#define rtc_hpp
#include <cstdint>

struct Alarm
{
    uint8_t day_of_week, hour, minute;
};

class RealTimeClock
{
    private:
        uint8_t stat1_reg;
        uint8_t stat2_reg;
        uint8_t year, month, day, day_of_week;
        uint8_t hour, minute, second;
    
        Alarm alarm1, alarm2;
    
        //Stuff related to bitbanging
        uint16_t io_reg;
        uint8_t internal_output[7];
        int command;
        int input;
        int input_bit_num;
        int input_index;
        int output_bit_num;
        int output_index;
    public:
        void init();
        void interpret_input();
        uint16_t read();
        void write(uint16_t value, bool is_byte);
};

#endif /* rtc_hpp */
