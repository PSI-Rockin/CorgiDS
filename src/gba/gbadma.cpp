#include "../emulator.hpp"
#include "gbadma.hpp"

GBA_DMA::GBA_DMA(Emulator* e) : e(e)
{

}

void GBA_DMA::power_on()
{
    active_DMAs = 0;
    for (unsigned int i = 0; i < 4; i++)
        dmas[i].CNT.set(0);
}

void GBA_DMA::handle_DMA(int index)
{
    DMA* active_DMA = &dmas[index];
    for (;;)
    {
        active_DMA->internal_len++;
        if (active_DMA->internal_len > active_DMA->length)
        {
            /*if (active_DMA->CNT.IRQ_after_transfer)
            {
                    e->request_interrupt7(static_cast<INTERRUPT>(4 + active_DMA->index));
            }*/
            active_DMA->internal_len = 0;
            active_DMAs &= ~(1 << index);
            //Reload dest
            if (active_DMA->CNT.dest_control == 3)
                active_DMA->internal_dest = active_DMA->destination;
            if (!active_DMA->CNT.repeat)
                active_DMA->CNT.enabled = false;
            return;
        }

        uint32_t value;
        int offset;
        if (active_DMA->CNT.word_transfer)
        {
            offset = 4;
            value = e->gba_read_word(active_DMA->internal_source);
            e->gba_write_word(active_DMA->internal_dest, value);
        }
        else
        {
            offset = 2;
            value = e->gba_read_halfword(active_DMA->internal_source);
            e->gba_write_halfword(active_DMA->internal_dest, value);
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
    }
}

uint16_t GBA_DMA::read_CNT(int index)
{
    return dmas[index].CNT.get();
}

uint32_t GBA_DMA::read_len_CNT(int index)
{
    return dmas[index].length | (dmas[index].CNT.get() << 16);
}

void GBA_DMA::write_source(int index, uint32_t source)
{
    dmas[index].source = source;
}

void GBA_DMA::write_dest(int index, uint32_t dest)
{
    dmas[index].destination = dest;
}

void GBA_DMA::write_len(int index, uint32_t len)
{
    if (index == 3)
    {
        if (!len)
            dmas[index].length = 0x10000;
        else
            dmas[index].length = len & 0xFFFF;
    }
    else
    {
        if (!len)
            dmas[index].length = 0x4000;
        else
            dmas[index].length = len & 0x3FFF;
    }
}

void GBA_DMA::write_CNT(int index, uint16_t CNT)
{
    bool old_enabled = dmas[index].CNT.enabled;
    dmas[index].CNT.set(CNT);
    if (!old_enabled && (CNT & (1 << 15)))
    {
        dmas[index].internal_source = dmas[index].source;
        dmas[index].internal_dest = dmas[index].destination;
        dmas[index].internal_len = 0;
        /*if (dmas[index].CNT.timing)
        {
            printf("\nDMA%d activated", index);
            printf("\nSource: $%08X", dmas[index].source);
            printf("\nDest: $%08X", dmas[index].destination);
            printf("\nLen: %d", dmas[index].length);
            printf("\nCNT: $%04X", dmas[index].CNT.get());
            printf("\nTiming: %d", dmas[index].CNT.timing);
        }*/

        if (dmas[index].CNT.timing == 0)
        {
            //printf("\nRunning immediately");
            handle_DMA(index);
        }
    }
}

void GBA_DMA::write_len_CNT(int index, uint32_t word)
{
    write_len(index, word & 0xFFFF);
    write_CNT(index, word >> 16);
}

void GBA_DMA::HBLANK_request()
{
    for (int i = 0; i < 4; i++)
    {
        if (dmas[i].CNT.enabled && (dmas[i].CNT.timing & 0x4) != 0)
            handle_DMA(i);
    }
}
