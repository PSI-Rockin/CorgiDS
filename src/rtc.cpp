/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "rtc.hpp"
#include <cstdio>
#include <cstdlib>
#include <ctime>

//https://stackoverflow.com/questions/1408361/unsigned-integer-to-bcd-conversion
uint8_t byte_to_BCD(uint8_t byte)
{
    return (byte / 10 * 16) + (byte % 10);
}

void RealTimeClock::init()
{
    //Place a hardcoded date into the RTC
    //For those curious, this is January 1st, 2005, a couple of months after the release of the NDS
    year = 0x05;
    month = 0x1;
    day = 0x1;
    day_of_week = 0x0;
    
    //Set the time to midnight
    hour = 0x12;
    minute = 0x0;
    second = 0x0;
    
    io_reg = 0;
    stat1_reg = 0;
    stat2_reg = 0;
}

void RealTimeClock::interpret_input()
{
    //printf("\nRTC input: $%02X", input);
    if (input_index == 0)
    {
        command = (input & 0x70) >> 4;
        if (input & 0x80) //Read from a register
        {
            switch (command)
            {
                case 0:
                    internal_output[0] = stat1_reg;
                    break;
                case 1:
                    if (stat2_reg & 0x4)
                    {
                        internal_output[0] = alarm1.day_of_week;
                        internal_output[1] = alarm1.hour;
                        internal_output[2] = alarm1.minute;
                    }
                    else
                    {
                        internal_output[0] = alarm1.minute;
                    }
                case 2:
                {
                    //Hacky solution but whatever
                    //https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
                    time_t t = time(0);
                    struct tm* now = localtime(&t);
                    internal_output[0] = byte_to_BCD(now->tm_year - 100);
                    internal_output[1] = byte_to_BCD(now->tm_mon + 1);
                    internal_output[2] = byte_to_BCD(now->tm_mday);
                    internal_output[3] = byte_to_BCD(now->tm_wday);
                    if (stat1_reg & 0x2)
                    {
                        //24 hour mode
                        internal_output[4] = byte_to_BCD(now->tm_hour);
                    }
                    else
                    {
                        //12 hour mode
                        internal_output[4] = byte_to_BCD(now->tm_hour % 12);
                    }
                    internal_output[4] |= (now->tm_hour >= 12) << 6;
                    internal_output[5] = byte_to_BCD(now->tm_min);
                    internal_output[6] = byte_to_BCD(now->tm_sec);
                }
                    /*internal_output[0] = year;
                    internal_output[1] = month;
                    internal_output[2] = day;
                    internal_output[3] = day_of_week;
                    internal_output[4] = hour;
                    internal_output[5] = minute;
                    internal_output[6] = second;*/
                    break;
                case 4:
                    internal_output[0] = stat2_reg;
                    break;
                case 5:
                    internal_output[0] = alarm2.day_of_week;
                    internal_output[1] = alarm2.hour;
                    internal_output[2] = alarm2.minute;
                    break;
                case 6:
                    internal_output[0] = hour;
                    internal_output[1] = minute;
                    internal_output[2] = second;
                    break;
                default:
                    printf("\nUnrecognized read from RTC command %d", command);
                    throw "[RTC] Unrecognized read";
            }
        }
    }
    else
    {
        switch (command)
        {
            case 0:
                stat1_reg = input;
                break;
            case 1:
                if (stat2_reg & 0x4)
                {
                    if (input_index == 1)
                        alarm1.day_of_week = input;
                    else if (input_index == 2)
                        alarm1.hour = input;
                    else if (input_index == 3)
                        alarm1.minute = input;
                }
                else
                {
                    if (input_index == 1)
                        alarm1.minute = input; //honestly I dunno
                }
                break;
            case 2:
                //do nothing for now
                break;
            case 3:
                //TODO: clock adjustment?
                break;
            case 4:
                if (input_index == 1)
                    stat2_reg = input;
                break;
            case 5:
                if (input_index == 1)
                    alarm2.day_of_week = input;
                else if (input_index == 2)
                    alarm2.hour = input;
                else if (input_index == 3)
                    alarm2.minute = input;
                break;
            default:
                printf("\nUnrecognized write $%02X to RTC command %d", input, command);
                throw "[RTC] Unrecognized write";
        }
    }
    input_index++;
}

uint16_t RealTimeClock::read()
{
    return io_reg;
}

void RealTimeClock::write(uint16_t value, bool is_byte)
{
    if (is_byte)
        value |= io_reg & 0xFF00;
    int data = value & 0x1;
    bool clock_out = value & 0x2;
    bool select_out = value & 0x4;
    bool is_writing = value & 0x10;
    
    if (select_out)
    {
        if (!(io_reg & 0x4))
        {
            input = 0;
            input_bit_num = 0;
            input_index = 0;
            output_bit_num = 0;
            output_index = 0;
        }
        else
        {
            if (!clock_out)
            {
                if (is_writing)
                {
                    input |= data << input_bit_num;
                    input_bit_num++;
                    
                    if (input_bit_num == 8)
                    {
                        input_bit_num = 0;
                        interpret_input();
                        input = 0;
                    }
                }
                else
                {
                    if (internal_output[output_index] & (1 << output_bit_num))
                        io_reg |= 0x1;
                    else
                        io_reg &= ~0x1;
                    output_bit_num++;
                    
                    if (output_bit_num == 8)
                    {
                        output_bit_num = 0;
                        if (output_index < 7) //Prevent buffer overflow
                            output_index++;
                    }
                }
            }
        }
    }
    
    if (is_writing)
        io_reg = value;
    else
        io_reg = (io_reg & 0x1) | (value & 0xFE);
}
