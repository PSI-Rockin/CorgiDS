/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef timers_hpp
#define timers_hpp
#include <cstdint>

//Enum representing the different frequencies available for timers
//F = system clock (33.513982 MHz on NDS)
enum DIVISOR
{
    F_1,
    F_64,
    F_256,
    F_1024
};

struct TimerReg
{
    uint16_t counter;
    uint16_t reload_value;
    int cycles_left;
    
    DIVISOR clock_div;
    bool count_up_timing; //If on, increment timer when previous overflows (can't be used for Timer 0)
    bool IRQ_on_overflow;
    bool enabled;
};

class Emulator;

class NDS_Timing
{
    private:
        Emulator* e;
        int timer_clock_divs[4];
        TimerReg timers[8];
    
        void overflow(int index);
    public:
        NDS_Timing(Emulator* e);
        void power_on();
        void run_timers9(int cycles);
        void run_timers7(int cycles);
        void run_timer(int cycles, int index);
    
        uint16_t read_lo(int index);
        uint16_t read_hi(int index);
    
        void write(uint32_t value, int index);
        void write_lo(uint16_t value, int index);
        void write_hi(uint16_t value, int index);
};

#endif /* timers_hpp */
