/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef interrupts_hpp
#define interrupts_hpp
#include <cstdint>

enum class INTERRUPT
{
    VBLANK,
    HBLANK,
    VCOUNT_MATCH,
    TIMER0,
    TIMER1,
    TIMER2,
    TIMER3,
    RTC,
    DMA0,
    DMA1,
    DMA2,
    DMA3,
    KEYPAD,
    GBA_SLOT,
    IPCSYNC = 16,
    IPC_FIFO_EMPTY,
    IPC_FIFO_NEMPTY,
    CART_TRANSFER,
    CART_IREQ_MC,
    GEOMETRY_FIFO,
    UNFOLD_SCREEN,
    SPI,
    WIFI
};

struct InterruptRegs
{
    uint32_t IME;
    uint32_t IE;
    uint32_t IF;
    
    bool is_requesting_int(int bit);
};

inline bool InterruptRegs::is_requesting_int(int bit)
{
    return (IE & bit) && (IF & bit);
}

#endif /* interrupts_hpp */
