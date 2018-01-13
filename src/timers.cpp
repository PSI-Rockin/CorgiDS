/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "emulator.hpp"
#include "timers.hpp"

NDS_Timing::NDS_Timing(Emulator* e) : e(e)
{
    timer_clock_divs[DIVISOR::F_1] = 1;
    timer_clock_divs[DIVISOR::F_64] = 64;
    timer_clock_divs[DIVISOR::F_256] = 256;
    timer_clock_divs[DIVISOR::F_1024] = 1024;
}

void NDS_Timing::overflow(int index)
{
    timers[index].counter += timers[index].reload_value;
    
    if (timers[index].IRQ_on_overflow)
    {
        if (index < 4)
        {
            int id = static_cast<int>(INTERRUPT::TIMER0) + index;
            e->request_interrupt7(static_cast<INTERRUPT>(id));
        }
        else
        {
            int id = static_cast<int>(INTERRUPT::TIMER0) + index - 4;
            e->request_interrupt9(static_cast<INTERRUPT>(id));
        }
    }
    
    //Count-up timing behavior
    if (index != 3 && index != 7)
    {
        if (timers[index + 1].count_up_timing && timers[index + 1].enabled)
        {
            if (timers[index + 1].counter == 0xFFFF)
                overflow(index + 1); //recursion baby
            else
                timers[index + 1].counter++;
        }
    }
}

void NDS_Timing::power_on()
{
    for (int i = 0; i < 8; i++)
    {
        timers[i].enabled = false;
        timers[i].IRQ_on_overflow = false;
    }
}

void NDS_Timing::run_timers7(int cycles)
{
    if (timers[0].enabled)
        run_timer(cycles, 0);
    if (timers[1].enabled)
        run_timer(cycles, 1);
    if (timers[2].enabled)
        run_timer(cycles, 2);
    if (timers[3].enabled)
        run_timer(cycles, 3);
}

void NDS_Timing::run_timers9(int cycles)
{
    if (timers[4].enabled)
        run_timer(cycles, 4);
    if (timers[5].enabled)
        run_timer(cycles, 5);
    if (timers[6].enabled)
        run_timer(cycles, 6);
    if (timers[7].enabled)
        run_timer(cycles, 7);
}

void NDS_Timing::run_timer(int cycles, int index)
{
    if (!timers[index].count_up_timing)
    {
        timers[index].cycles_left -= cycles;

        while (timers[index].cycles_left <= 0)
        {
            timers[index].counter++;
            timers[index].cycles_left += timer_clock_divs[timers[index].clock_div];
            if (!timers[index].counter)
                overflow(index);
        }
    }
}

uint16_t NDS_Timing::read_lo(int index)
{
    return timers[index].counter;
}

uint16_t NDS_Timing::read_hi(int index)
{
    uint16_t reg = 0;
    reg |= timers[index].clock_div;
    reg |= timers[index].count_up_timing << 2;
    reg |= timers[index].IRQ_on_overflow << 6;
    reg |= timers[index].enabled << 7;
    return reg;
}

void NDS_Timing::write(uint32_t value, int index)
{
    write_lo(value & 0xFFFF, index);
    write_hi(value >> 16, index);
}

void NDS_Timing::write_lo(uint16_t value, int index)
{
    timers[index].reload_value = value;
}

void NDS_Timing::write_hi(uint16_t value, int index)
{
    switch (value & 0x3)
    {
        case 0:
            timers[index].clock_div = DIVISOR::F_1;
            timers[index].cycles_left = 1;
            break;
        case 1:
            timers[index].clock_div = DIVISOR::F_64;
            timers[index].cycles_left = 64;
            break;
        case 2:
            timers[index].clock_div = DIVISOR::F_256;
            timers[index].cycles_left = 256;
            break;
        case 3:
            timers[index].clock_div = DIVISOR::F_1024;
            timers[index].cycles_left = 1024;
            break;
    }
    
    timers[index].count_up_timing = value & (1 << 2);
    timers[index].IRQ_on_overflow = value & (1 << 6);
    
    //If timer is being newly enabled, reload the counter
    if (!timers[index].enabled && (value & (1 << 7)))
        timers[index].counter = timers[index].reload_value;
    
    timers[index].enabled = value & (1 << 7);
}
