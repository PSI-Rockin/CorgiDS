/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "emulator.hpp"
#include "spu.hpp"

#define printf(fmt, ...)(0)

const int volume_shift[4] = {4, 3, 2, 0};

const int ADPCM_indexes[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

const uint16_t ADPCM_samples[89] =
{
    0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E,
    0x0010, 0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F,
    0x0022, 0x0025, 0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042,
    0x0049, 0x0050, 0x0058, 0x0061, 0x006B, 0x0076, 0x0082, 0x008F,
    0x009D, 0x00AD, 0x00BE, 0x00D1, 0x00E6, 0x00FD, 0x0117, 0x0133,
    0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE, 0x0220, 0x0256, 0x0292,
    0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E, 0x0502, 0x0583,
    0x0610, 0x06AB, 0x0756, 0x0812, 0x08E0, 0x09C3, 0x0ABD, 0x0BD0,
    0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954,
    0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B,
    0x3BB9, 0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462,
    0x7FFF
};

const int16_t PSG_samples[8][8] =
{
    {-0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF,  0x7FFF},
    {-0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF,  0x7FFF,  0x7FFF},
    {-0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF},
    {-0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF},
    {-0x7FFF, -0x7FFF, -0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF},
    {-0x7FFF, -0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF},
    {-0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF,  0x7FFF},
    {-0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF}
};

void SoundChannel::start()
{
    timer = SOUND_TIMER;
    current_sample = 0;
    white_noise = 0x7FFF;

    //One sample delay for PSG/Noise, three for PCM/ADPCM
    //ADPCM technically has eleven sample delay, due to header
    if (CNT.format == 3)
        current_position = -1;
    else
        current_position = -3;
}

void SoundChannel::run(int32_t& output, int id)
{
    output = 0;
    timer += 512; //512 cycles per sample; SPU runs at half ARM7 clock speed, so ~16 MHz
    while (timer >> 16)
    {
        timer = SOUND_TIMER + (timer - 0x10000);
        switch (CNT.format)
        {
            case 0:
                generate_sample_PCM8();
                break;
            case 1:
                generate_sample_PCM16();
                break;
            case 2:
                generate_sample_ADPCM();
                break;
            case 3:
                if (id >= 8 && id <= 13)
                    generate_sample_PSG();
                if (id >= 14)
                    generate_sample_noise();
                break;
            default:
                printf("\nUnrecognized sound format %d", CNT.format);
                break;
        }
    }

    int32_t final_sample = (int32_t)current_sample;
    final_sample <<= volume_shift[CNT.divider];
    final_sample *= CNT.volume + ((CNT.volume & 0x7F) == 0x7F);
    output = final_sample;
}

void SoundChannel::pan_output(int32_t &input, int32_t &left, int32_t &right)
{
    int64_t bark = (int64_t)(int32_t)input;

    left += (bark * (128 - CNT.panning)) >> 10;
    right += (bark * CNT.panning) >> 10;
}

void SoundChannel::generate_sample_PCM8()
{
    current_position++;
    if (current_position < 0) //Delay if necessary
        return;

    if (current_position >= SOUND_PNT + SOUND_LEN)
    {
        if (CNT.repeat_mode == 1)
            current_position = SOUND_PNT;
        else if (CNT.repeat_mode == 2)
        {
            current_sample = 0;
            CNT.busy = false;
        }
    }

    current_sample = (int8_t)e->arm7_read_byte(SOUND_SOURCE + current_position) << 8;
}

void SoundChannel::generate_sample_PCM16()
{
    current_position++;
    if (current_position < 0) //Delay if necessary
        return;

    if ((current_position * 2) >= SOUND_PNT + SOUND_LEN)
    {
        if (CNT.repeat_mode == 1)
            current_position = SOUND_PNT / 2;
        else if (CNT.repeat_mode == 2)
        {
            current_sample = 0;
            CNT.busy = false;
        }
    }

    current_sample = (int16_t)e->arm7_read_halfword(SOUND_SOURCE + (current_position * 2));
}

void SoundChannel::generate_sample_ADPCM()
{
    current_position++;
    if (current_position < 8)
    {
        if (!current_position)
        {
            //Load header
            uint32_t header = e->arm7_read_word(SOUND_SOURCE);
            ADPCM_sample = header & 0xFFFF;
            ADPCM_index = (header >> 16) & 0xFF;
            if (ADPCM_index > 88)
                ADPCM_index = 88;

            ADPCM_sample_loop = ADPCM_sample;
            ADPCM_index_loop = ADPCM_index;
        }
        return;
    }

    if ((current_position / 2) >= SOUND_PNT + SOUND_LEN)
    {
        if (CNT.repeat_mode == 1)
        {
            ADPCM_sample = ADPCM_sample_loop;
            ADPCM_index = ADPCM_index_loop;
            current_position = SOUND_PNT * 2;
        }
        else if (CNT.repeat_mode == 2)
        {
            current_sample = 0;
            CNT.busy = false;
        }
    }
    else
    {
        uint8_t ADPCM_byte = e->arm7_read_byte(SOUND_SOURCE + (current_position / 2));
        if (current_position & 0x1)
            ADPCM_byte >>= 4;
        else
            ADPCM_byte &= 0xF;

        uint16_t ADPCM_value = ADPCM_samples[ADPCM_index];
        uint16_t diff = ADPCM_value / 8;
        if (ADPCM_byte & 0x1)
            diff += ADPCM_value / 4;
        if (ADPCM_byte & 0x2)
            diff += ADPCM_value / 2;
        if (ADPCM_byte & 0x4)
            diff += ADPCM_value;
        if (ADPCM_byte & 0x8)
        {
            ADPCM_sample -= diff;
            if (ADPCM_sample < -0x7FFF)
                ADPCM_sample = -0x7FFF;
        }
        else
        {
            ADPCM_sample += diff;
            if (ADPCM_sample > 0x7FFF)
                ADPCM_sample = 0x7FFF;
        }

        ADPCM_index += ADPCM_indexes[ADPCM_byte & 0x7];
        if (ADPCM_index < 0)
            ADPCM_index = 0;
        if (ADPCM_index > 88)
            ADPCM_index = 88;

        if ((current_position / 2) == SOUND_PNT)
        {
            ADPCM_sample_loop = ADPCM_sample;
            ADPCM_index_loop = ADPCM_index;
        }
    }

    current_sample = ADPCM_sample;
}

void SoundChannel::generate_sample_PSG()
{
    current_position++;
    current_sample = PSG_samples[CNT.wave_duty][current_position & 0x7];
}

void SoundChannel::generate_sample_noise()
{
    bool carry = white_noise & 0x1;
    white_noise >>= 1;
    if (carry)
    {
        white_noise ^= 0x6000;
        current_sample = -0x7FFF;
    }
    else
        current_sample = 0x7FFF;
}

SPU::SPU(Emulator *e) : e(e)
{
    for (int i = 0; i < 16; i++)
        channels[i].e = e;

    power_on();
}

void SPU::power_on()
{
    SOUNDCNT.master_volume = 0;
    SOUNDCNT.left_output = 0;
    SOUNDCNT.right_output = 0;
    SOUNDCNT.output_ch1_mixer = false;
    SOUNDCNT.output_ch3_mixer = false;
    SOUNDCNT.master_enable = false;
    SOUNDBIAS = 0x200;
    samples = 0;

    for (int i = 0; i < 16; i++)
    {
        channels[i].CNT.busy = false;
        channels[i].CNT.volume = 0;
        channels[i].timer = 0;
        channels[i].SOUND_TIMER = 0;
        channels[i].SOUND_LEN = 0;
        channels[i].SOUND_PNT = 0;
        channels[i].SOUND_SOURCE = 0;
    }
}

void SPU::generate_sample(uint64_t cycles)
{
    if (SOUNDCNT.master_enable && samples < SAMPLE_BUFFER_SIZE)
    {
        int32_t channel_buffer = 0, left_sample = 0, right_sample = 0;
        cycle_diff += cycles;

        //Generate one sample for every 1024 ARM7 cycles
        while (cycle_diff >= 1024)
        {
            cycle_diff -= 1024;
            for (int i = 0; i < 16; i++)
            {
                if (channels[i].CNT.busy)
                {
                    channels[i].run(channel_buffer, i);
                    channels[i].pan_output(channel_buffer, left_sample, right_sample);
                }
            }
            left_sample = ((int64_t)left_sample * SOUNDCNT.master_volume) >> 7;
            right_sample = ((int64_t)right_sample * SOUNDCNT.master_volume) >> 7;

            //Convert samples to PCM16 and clip
            left_sample >>= 8;
            right_sample >>= 8;
            if (left_sample < -0x8000)
                left_sample = -0x8000;
            if (left_sample > 0x7FFF)
                left_sample = 0x7FFF;

            if (right_sample < -0x8000)
                right_sample = -0x8000;
            if (right_sample > 0x7FFF)
                right_sample = 0x7FFF;

            sample_buffer[samples] = left_sample >> 1;
            sample_buffer[samples + 1] = right_sample >> 1;

            samples += 2;
        }
    }
}

int SPU::output_buffer(int16_t *data)
{
    for (int i = 0; i < samples; i++)
        data[i] = sample_buffer[i];
    //memcpy(data, sample_buffer, samples * sizeof(int16_t));
    int samples_out = samples;
    samples = 0;
    return samples_out;
}

uint8_t SPU::read_channel_byte(uint32_t address)
{
    printf("\n[SPU] Byte read from $%08X", address);
    address -= 0x04000400;
    int i = (address >> 4) & 0xF;
    int reg_part = address & 0xF;

    uint8_t reg = 0;

    switch (reg_part)
    {
        case 0:
            reg = channels[i].CNT.volume & 0x7F;
            break;
        case 1:
            reg = channels[i].CNT.divider & 0x3;
            reg |= channels[i].CNT.hold_sample << 7;
            break;
        case 2:
            reg = channels[i].CNT.panning & 0x7F;
            break;
        case 3:
            reg = channels[i].CNT.wave_duty & 0x7;
            reg |= (channels[i].CNT.repeat_mode & 0x3) << 3;
            reg |= (channels[i].CNT.format & 0x3) << 5;
            reg |= channels[i].CNT.busy << 7;
            break;
    }

    return reg;
}

uint32_t SPU::read_channel_word(uint32_t address)
{
    printf("\n[SPU] Word read from $%08X", address);

    uint32_t reg = 0;
    int i = (address >> 4) & 0xF;
    int reg_part = address & 0xF;

    switch (reg_part)
    {
        case 0:
            reg = channels[i].CNT.volume & 0x7F;
            reg |= (channels[i].CNT.divider & 0x3) << 8;
            reg |= channels[i].CNT.hold_sample << 15;
            reg |= (channels[i].CNT.panning & 0x7F) << 16;
            reg |= (channels[i].CNT.wave_duty & 0x7) << 24;
            reg |= (channels[i].CNT.repeat_mode & 0x3) << 27;
            reg |= (channels[i].CNT.format & 0x3) << 29;
            reg |= channels[i].CNT.busy << 31;
            break;
    }
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

int SPU::get_samples()
{
    return samples;
}

void SPU::write_channel_byte(uint32_t address, uint8_t byte)
{
    int i = (address & 0xF0) >> 4;
    switch (address & 0xF)
    {
        case 0x0:
            channels[i].CNT.volume = byte & 0x7F;
            break;
        case 0x1:
            channels[i].CNT.divider = byte & 0x3;
            channels[i].CNT.hold_sample = byte & (1 << 7);
            break;
        case 0x2:
            channels[i].CNT.panning = byte & 0x7F;
            break;
        case 0x3:
            channels[i].CNT.wave_duty = byte & 0x7;
            channels[i].CNT.repeat_mode = (byte >> 3) & 0x3;
            channels[i].CNT.format = (byte >> 5) & 0x3;
            if (!channels[i].CNT.busy && (byte & (1 << 7)))
                channels[i].start();
            channels[i].CNT.busy = byte & (1 << 7);
            break;
    }
    if (byte)
        printf("\n[SPU] Byte write to $%08X of $%02X", address, byte);
}

void SPU::write_channel_halfword(uint32_t address, uint16_t halfword)
{
    int i = (address & 0xF0) >> 4;
    switch (address & 0xF)
    {
        case 0x0:
            channels[i].CNT.volume = halfword & 0x7F;
            channels[i].CNT.divider = (halfword >> 8) & 0x3;
            channels[i].CNT.hold_sample = halfword & (1 << 15);
            break;
        case 0x2:
            channels[i].CNT.panning = halfword & 0x7F;
            channels[i].CNT.wave_duty = (halfword >> 8) & 0x7;
            channels[i].CNT.repeat_mode = (halfword >> 11) & 0x3;
            channels[i].CNT.format = (halfword >> 13) & 0x3;
            if (!channels[i].CNT.busy && (halfword & (1 << 15)))
                channels[i].start();
            channels[i].CNT.busy = halfword & (1 << 15);
            break;
        case 0x8:
            channels[i].SOUND_TIMER = halfword;
            break;
        case 0xA:
            channels[i].SOUND_PNT = halfword * 4;
            break;
    }
    if (halfword)
        printf("\n[SPU] Halfword write to $%08X of $%04X", address, halfword);
}

void SPU::write_channel_word(uint32_t address, uint32_t word)
{
    int i = (address & 0xF0) >> 4;
    switch (address & 0xF)
    {
        case 0x0:
            channels[i].CNT.volume = word & 0x7F;
            channels[i].CNT.divider = (word >> 8) & 0x3;
            channels[i].CNT.hold_sample = word & (1 << 15);
            channels[i].CNT.panning = (word >> 16) & 0x7F;
            channels[i].CNT.wave_duty = (word >> 24) & 0x7;
            channels[i].CNT.repeat_mode = (word >> 27) & 0x3;
            channels[i].CNT.format = (word >> 29) & 0x3;
            if (!channels[i].CNT.busy && (word & (1 << 31)))
                channels[i].start();
            channels[i].CNT.busy = word & (1 << 31);
            break;
        case 0x4:
            channels[i].SOUND_SOURCE = word & 0x07FFFFFC;
            break;
        case 0xC:
            channels[i].SOUND_LEN = (word & 0x1FFFFF) * 4;
            break;
    }
    if (word)
        printf("\n[SPU] Word write to $%08X of $%08X", address, word);
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
