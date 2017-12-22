/*
    CorgiDS Copyright PSISP 2017
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdlib>
#include "spu.hpp"

void SPU::power_on()
{
    SOUNDCNT.master_volume = 0;
    SOUNDCNT.left_output = 0;
    SOUNDCNT.right_output = 0;
    SOUNDCNT.output_ch1_mixer = false;
    SOUNDCNT.output_ch3_mixer = false;
    SOUNDCNT.master_enable = false;
    SOUNDBIAS = 0x200;
}

uint8_t SPU::read_channel_byte(uint32_t address)
{
    address -= 0x04000400;
    int index = (address >> 4) & 0xF;
    int opcode = address & 0xF;

    uint8_t reg = 0;

    return reg;
}

uint16_t SPU::get_SOUNDCNT()
{
    uint16_t reg = 0;
    reg |= SOUNDCNT.master_volume;
    reg |= SOUNDCNT.left_output << 8;
    reg |= SOUNDCNT.right_output << 10;
    reg |= SOUNDCNT.output_ch1_mixer << 12;
    reg |= SOUNDCNT.output_ch3_mixer << 13;
    reg |= SOUNDCNT.master_enable << 15;
    return reg;
}

uint16_t SPU::get_SOUNDBIAS()
{
    return SOUNDBIAS;
}

uint8_t SPU::get_SNDCAP0()
{
    return 0;
}

uint8_t SPU::get_SNDCAP1()
{
    return 0;
}

void SPU::write_channel_byte(uint32_t address, uint8_t byte)
{

}

void SPU::write_channel_halfword(uint32_t address, uint16_t halfword)
{

}

void SPU::write_channel_word(uint32_t address, uint32_t word)
{

}

void SPU::set_SOUNDCNT_lo(uint8_t byte)
{
    SOUNDCNT.master_volume = byte & 0x7F;
}

void SPU::set_SOUNDCNT_hi(uint8_t byte)
{
    SOUNDCNT.left_output = byte & 0x3;
    SOUNDCNT.right_output = (byte >> 2) & 0x3;
    SOUNDCNT.output_ch1_mixer = byte & (1 << 4);
    SOUNDCNT.output_ch3_mixer = byte & (1 << 5);
    SOUNDCNT.master_enable = byte & (1 << 7);
}

void SPU::set_SOUNDCNT(uint16_t halfword)
{
    set_SOUNDCNT_lo(halfword & 0xFF);
    set_SOUNDCNT_hi(halfword >> 8);
}

void SPU::set_SOUNDBIAS(uint16_t value)
{
    SOUNDBIAS = value & 0x3FF;
}

void SPU::set_SNDCAP0(uint8_t value)
{

}

void SPU::set_SNDCAP1(uint8_t value)
{

}
