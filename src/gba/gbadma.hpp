#ifndef DMA_HPP
#define DMA_HPP
#include "../dma.hpp"

class GBA_DMA
{
    private:
        Emulator* e;
        uint8_t active_DMAs;
        DMA dmas[4];
    public:
        GBA_DMA(Emulator* e);

        void power_on();
        void handle_DMA(int index);

        uint16_t read_CNT(int index);
        uint32_t read_len_CNT(int index);

        void write_source(int index, uint32_t source);
        void write_dest(int index, uint32_t dest);
        void write_len(int index, uint32_t len);
        void write_CNT(int index, uint16_t CNT);
        void write_len_CNT(int index, uint32_t word);

        void HBLANK_request();
};

#endif // GBADMA_HPP
