/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "dma.hpp"
#include <cstdio>
#include "emulator.hpp"

uint16_t DMACNT::get()
{
    uint16_t reg = 0;
    reg |= dest_control << 5;
    reg |= source_control << 7;
    reg |= repeat << 9;
    reg |= word_transfer << 10;
    reg |= timing << 11;
    reg |= IRQ_after_transfer << 14;
    reg |= enabled << 15;
    return reg;
}

void DMACNT::set(uint16_t value)
{
    dest_control = (value >> 5) & 0x3;
    source_control = (value >> 7) & 0x3;
    repeat = value & (1 << 9);
    word_transfer = value & (1 << 10);
    timing = (value >> 11) & 0x7;
    IRQ_after_transfer = value & (1 << 14);
    enabled = value & (1 << 15);
}

NDS_DMA::NDS_DMA(Emulator* e) : e(e)
{
    for (unsigned int i = 0; i < 8; i++)
    {
        dmas[i].is_arm9 = i < 4;
        dmas[i].index = i;
    }
}

void NDS_DMA::power_on()
{
    active_DMAs = 0;
    for (unsigned int i = 0; i < 8; i++)
    {
        dmas[i].is_arm9 = i < 4;
        dmas[i].CNT.set(0);
    }
}

void NDS_DMA::activate_DMA(int index)
{
    //TODO: add proper scheduling infrastructure
    active_DMAs |= (1 << index);
    SchedulerEvent event;
    event.id = index;
    handle_event(event);
}

void NDS_DMA::handle_event(SchedulerEvent &event)
{
    event.processing = false;
    DMA* active_DMA = &dmas[event.id];
    for (;;)
    {
        active_DMA->internal_len++;
        if (active_DMA->internal_len > active_DMA->length)
        {
            if (active_DMA->CNT.IRQ_after_transfer)
            {
                if (active_DMA->is_arm9)
                    e->request_interrupt9(static_cast<INTERRUPT>(8 + active_DMA->index));
                else
                    e->request_interrupt7(static_cast<INTERRUPT>(4 + active_DMA->index));
            }
            active_DMA->internal_len = 0;
            active_DMAs &= ~(1 << event.id);
            //Reload dest
            if (active_DMA->CNT.dest_control == 3)
                active_DMA->internal_dest = active_DMA->destination;
            if (!active_DMA->CNT.repeat)
                active_DMA->CNT.enabled = false;
            return;
        }

        int value;
        int offset;
        if (active_DMA->CNT.word_transfer)
        {
            offset = 4;
            if (active_DMA->is_arm9)
            {
                value = e->arm9_read_word(active_DMA->internal_source);
                e->arm9_write_word(active_DMA->internal_dest, value);
            }
            else
            {
                value = e->arm7_read_word(active_DMA->internal_source);
                e->arm7_write_word(active_DMA->internal_dest, value);
            }
        }
        else
        {
            offset = 2;
            if (active_DMA->is_arm9)
            {
                value = e->arm9_read_halfword(active_DMA->internal_source);
                e->arm9_write_halfword(active_DMA->internal_dest, value);
            }
            else
            {
                value = e->arm7_read_halfword(active_DMA->internal_source);
                e->arm7_write_halfword(active_DMA->internal_dest, value);
            }
        }

        switch (active_DMA->CNT.dest_control)
        {
            case 0:
            case 3:
                active_DMA->internal_dest += offset;
                break;
            case 1:
                active_DMA->internal_dest -= offset;
                break;
            case 2:
                //Fixed
                break;
            default:
                printf("\nUnrecognized DMA dest control %d", active_DMA->CNT.dest_control);
        }
        switch (active_DMA->CNT.source_control)
        {
            case 0:
                active_DMA->internal_source += offset;
                break;
            case 1:
                active_DMA->internal_source -= offset;
                break;
            case 2:
                //Fixed
                break;
            default:
                printf("\nUnrecognized DMA source control %d", active_DMA->CNT.source_control);
        }
        if (active_DMA->is_arm9 && active_DMA->CNT.timing == 7 && active_DMA->internal_len >= 112)
        {
            active_DMA->internal_len = 0;
            active_DMA->length -= 112;
            active_DMAs &= ~(1 << event.id);
            return;
        }
    }
}

void NDS_DMA::DMA_event(int index)
{
    DMA* active_DMA = &dmas[index];
    while (active_DMA->internal_len > active_DMA->length);
}

uint32_t NDS_DMA::read_source(int index)
{
    return dmas[index].source;
}

uint16_t NDS_DMA::read_len(int index)
{
    return dmas[index].length;
}

uint16_t NDS_DMA::read_CNT(int index)
{
    return dmas[index].CNT.get();
}

void NDS_DMA::write_source(int index, uint32_t source)
{
    dmas[index].source = source;
}

void NDS_DMA::write_dest(int index, uint32_t dest)
{
    dmas[index].destination = dest;
}

void NDS_DMA::write_len(int index, uint32_t len)
{
    if (dmas[index].is_arm9)
    {
        if (len == 0)
            dmas[index].length = 0x200000;
        else
            dmas[index].length = len & 0x1FFFFF;
    }
    else
    {
        if (len == 0)
            dmas[index].length = (index == 7) ? 0x10000 : 0x4000;
        else
            dmas[index].length = (index == 7) ? len & 0xFFFF : 0x3FFF;
    }
}

void NDS_DMA::write_CNT(int index, uint16_t CNT)
{
    bool old_enabled = dmas[index].CNT.enabled;
    dmas[index].CNT.set(CNT);
    if (!old_enabled && (CNT & (1 << 15)))
    {
        dmas[index].internal_source = dmas[index].source;
        dmas[index].internal_dest = dmas[index].destination;
        dmas[index].internal_len = 0;
        /*if (dmas[index].CNT.timing || true)
        {
            printf("\nDMA%d activated", index);
            printf("\nSource: $%08X", dmas[index].source);
            printf("\nDest: $%08X", dmas[index].destination);
            printf("\nLen: %d", dmas[index].length);
            printf("\nCNT: $%04X", dmas[index].CNT.get());
            printf("\nTiming: %d", dmas[index].CNT.timing);
        }*/

        /*if (dmas[index].destination < 0x02000000 || dmas[index].source < 0x02000000 || dmas[index].destination > 0x08000000)
            printf("\nWarning! DMA writing to unmapped memory");*/
        
        if (dmas[index].CNT.timing == 0)
        {
            //printf("\nRunning immediately");
            activate_DMA(index);
        }
        else if (dmas[index].CNT.timing == 7)
        {
            e->check_GXFIFO_DMA();
        }
    }
}

void NDS_DMA::write_len_CNT(int index, uint32_t word)
{
    write_len(index, word & 0x1FFFFF);
    write_CNT(index, word >> 16);
}

void NDS_DMA::HBLANK_request()
{
    for (int i = 0; i < 4; i++)
    {
        if (dmas[i].CNT.enabled && dmas[i].CNT.timing == 2 && !active_DMAs)
        {
            activate_DMA(i);
            break;
        }
    }
}

void NDS_DMA::gamecart_request()
{
    //NDS7 DMAs
    if (e->arm7_has_cart_rights())
    {
        for (int i = 4; i < 8; i++)
        {
            if (dmas[i].CNT.enabled && dmas[i].CNT.timing == 4)
            {
                activate_DMA(i);
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            if (dmas[i].CNT.enabled && dmas[i].CNT.timing == 5)
            {
                activate_DMA(i);
                break;
            }
        }
    }
}

void NDS_DMA::GXFIFO_request()
{
    for (int i = 0; i < 4; i++)
    {
        if (dmas[i].CNT.enabled && dmas[i].CNT.timing == 7)
        {
            activate_DMA(i);
            break;
        }
    }
}
