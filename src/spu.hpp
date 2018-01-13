/*
    CorgiDS Copyright PSISP 2017-2018
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

class Emulator;

struct SoundChannel
{
    Emulator* e;
    CHANNELCNT_REG CNT;
    uint32_t SOUND_SOURCE;
    uint16_t SOUND_TIMER; //reload
    uint16_t SOUND_PNT;
    uint16_t SOUND_LEN;

    uint32_t timer;
    int current_position;
    int16_t current_sample;

    int32_t ADPCM_sample, ADPCM_sample_loop;
    int32_t ADPCM_index, ADPCM_index_loop;

    uint16_t white_noise;

    void start();
    void run(int32_t& output, int id);
    void pan_output(int32_t& input, int32_t& left, int32_t& right);

    void generate_sample_PCM8();
    void generate_sample_PCM16();
    void generate_sample_ADPCM();
    void generate_sample_PSG();
    void generate_sample_noise();
};

class SPU
{
    public:
        constexpr static int SAMPLE_BUFFER_SIZE = 2048 * 2;
    private:
        Emulator* e;
        int cycle_diff;
        SoundChannel channels[16];
        SOUNDCNT_REG SOUNDCNT;
        SNDCAPTURE SNDCAP0, SNDCAP1;
        uint16_t SOUNDBIAS;

        int16_t sample_buffer[SAMPLE_BUFFER_SIZE];
        int samples;
    public:
        SPU(Emulator* e);
        void power_on();
        void generate_sample(uint64_t cycles);
        int output_buffer(int16_t* data);

        uint8_t read_channel_byte(uint32_t address);
        uint32_t read_channel_word(uint32_t address);
        uint16_t get_SOUNDCNT();
        uint16_t get_SOUNDBIAS();
        uint8_t get_SNDCAP0();
        uint8_t get_SNDCAP1();
        int get_samples();

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
