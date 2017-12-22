/*
    CorgiDS Copyright PSISP 2017
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef SPU_HPP
#define SPU_HPP
#include <cstdint>

struct CHANNELCNT_REG
{
    int volume;
    int divider;
    bool hold_sample;
    int panning;
    int wave_duty;
    int repeat_mode;
    int format;
    bool busy;
};

struct SoundChannel
{
    CHANNELCNT_REG CHANNELCNT;
    uint32_t SOUND_SOURCE;
    uint16_t SOUND_TIMER;
    uint16_t SOUND_PNT;
    uint16_t SOUND_LEN;
};

struct SOUNDCNT_REG
{
    int master_volume;
    int left_output;
    int right_output;
    bool output_ch1_mixer;
    bool output_ch3_mixer;

    bool master_enable;
};

struct SNDCAPTURE
{
    bool add_to_channel;
    bool capture_source;
    bool one_shot;
    bool capture_PCM8;
    bool busy;
    uint32_t destination;
    uint16_t len;
};

class SPU
{
    private:
        SoundChannel channels[16];
        SOUNDCNT_REG SOUNDCNT;
        SNDCAPTURE SNDCAP0, SNDCAP1;
        uint16_t SOUNDBIAS;
    public:
        void power_on();

        uint8_t read_channel_byte(uint32_t address);
        uint16_t get_SOUNDCNT();
        uint16_t get_SOUNDBIAS();
        uint8_t get_SNDCAP0();
        uint8_t get_SNDCAP1();
        void write_channel_byte(uint32_t address, uint8_t byte);
        void write_channel_halfword(uint32_t address, uint16_t halfword);
        void write_channel_word(uint32_t address, uint32_t word);
        void set_SOUNDCNT_lo(uint8_t byte);
        void set_SOUNDCNT_hi(uint8_t byte);
        void set_SOUNDCNT(uint16_t halfword);
        void set_SOUNDBIAS(uint16_t value);

        void set_SNDCAP0(uint8_t value);
        void set_SNDCAP1(uint8_t value);
};

#endif // SPU_HPP
