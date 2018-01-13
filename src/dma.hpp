/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef dma_hpp
#define dma_hpp
#include <cstdint>
#include <cstdio>

struct DMACNT
{
    int dest_control;
    int source_control;
    bool repeat;
    bool word_transfer;
    int timing;
    bool IRQ_after_transfer;
    bool enabled;
    
    uint16_t get();
    void set(uint16_t value);
};

//Note: Internal values are directly manipulated by the DMA, but the regular values are manipulated
//by hardware
struct DMA
{
    uint32_t source, internal_source;
    uint32_t destination, internal_dest;
    int length, internal_len;
    DMACNT CNT;
    int index;
    bool is_arm9;
};

class Emulator;
struct SchedulerEvent;

class NDS_DMA
{
    private:
        Emulator* e;
        DMA dmas[8];
        DMA* active_DMA7, active_DMA9;
        uint8_t active_DMAs;
    public:
        NDS_DMA(Emulator* e);
        void power_on();
        void DMA_event(int index);
        void update_DMA(int index);

        void activate_DMA(int index);
        void handle_event(SchedulerEvent& event);

        bool is_active(int cpu_id);
    
        uint32_t read_source(int index);
        uint16_t read_len(int index);
        uint16_t read_CNT(int index);
    
        void write_source(int index, uint32_t source);
        void write_dest(int index, uint32_t dest);
        void write_len(int index, uint32_t len);
        void write_CNT(int index, uint16_t CNT);
        void write_len_CNT(int index, uint32_t word);
    
        void HBLANK_request();
        void gamecart_request();
        void GXFIFO_request();
};

inline bool NDS_DMA::is_active(int cpu_id)
{
    return active_DMAs & (0xF << (cpu_id * 4));
}

#endif /* dma_hpp */
