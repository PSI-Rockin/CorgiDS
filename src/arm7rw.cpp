/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "emulator.hpp"

uint32_t Emulator::arm7_read_word(uint32_t address)
{
    //TODO: bad hack.
    if (gba_mode)
        return gba_read_word(address);
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
        return *(uint32_t*)&main_RAM[address & MAIN_RAM_MASK];
    if (address >= ARM7_WRAM_START && address < IO_REGS_START)
        return *(uint32_t*)&arm7_WRAM[address & ARM7_WRAM_MASK];
    if (address >= SHARED_WRAM_START && address < ARM7_WRAM_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Mirror to ARM7 WRAM
                return *(uint32_t*)&arm7_WRAM[address & ARM7_WRAM_MASK];
            case 1: //First half
                return *(uint32_t*)&shared_WRAM[address & 0x3FFF];
            case 2: //Second half
                return *(uint32_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000];
            case 3: //Entire 32 KB
                return *(uint32_t*)&shared_WRAM[address & 0x7FFF];
        }
    }
    switch (address)
    {
        case 0x040000DC:
            return dma.read_len(7) | dma.read_CNT(7) << 16;
        case 0x04000120:
            return 0;
        case 0x04000130:
            return KEYINPUT.get();
        case 0x04000180:
            return IPCSYNC_NDS7.read();
        case 0x040001A4:
            return cart.get_ROMCTRL();
        case 0x040001C0:
            return spi.get_SPICNT() | (spi.read_SPIDATA() << 16);
        case 0x04000208:
            return int7_reg.IME;
        case 0x04000210:
            return int7_reg.IE;
        case 0x04000214:
            return int7_reg.IF;
        case 0x04000218:
            return 0;
        case 0x04000500:
            return 0;
        case 0x04004008:
            return 0;
        case 0x04100000:
        {
            uint32_t word = fifo9.read_queue();
            if (fifo9.request_empty_IRQ)
            {
                request_interrupt9(INTERRUPT::IPC_FIFO_EMPTY);
                fifo9.request_empty_IRQ = false;
            }
            return word;
        }
        case 0x04100010:
            return cart.get_output();
    }
    if (address >= 0x04000400 && address < 0x04000500)
        return spu.read_channel_word(address);
    if (address < 0x4000)
    {
        if (arm7.get_PC() > 0x4000)
            return 0xFFFFFFFF;
        if (address < BIOSPROT && arm7.get_PC() > BIOSPROT)
            return 0xFFFFFFFF;
        return *(uint32_t*)&arm7_bios[address];
    }
    if (address >= 0x06000000 && address < 0x07000000)
        return gpu.read_ARM7<uint32_t>(address);
    if (address >= GBA_ROM_START && address < GBA_RAM_START)
        return slot2.read<uint32_t>(address);
    printf("\n(7) Unrecognized word read from $%08X", address);
    //exit(2);
    return 0;
}

uint16_t Emulator::arm7_read_halfword(uint32_t address)
{
    //TODO: bad hack.
    if (gba_mode)
        return gba_read_halfword(address);
    if (address < 0x4000)
    {
        if (arm7.get_PC() > 0x4000)
            return 0xFFFF;
        if (address < BIOSPROT && arm7.get_PC() > BIOSPROT)
            return 0xFFFF;
        return *(uint16_t*)&arm7_bios[address];
    }
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
        return *(uint16_t*)&main_RAM[address & MAIN_RAM_MASK];
    if (address >= SHARED_WRAM_START && address < ARM7_WRAM_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Mirror to ARM7 WRAM
                return *(uint16_t*)&arm7_WRAM[address & ARM7_WRAM_MASK];
            case 1: //First half
                return *(uint16_t*)&shared_WRAM[address & 0x3FFF];
            case 2: //Second half
                return *(uint16_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000];
            case 3: //Entire 32 KB
                return *(uint16_t*)&shared_WRAM[address & 0x7FFF];
        }
    }
    if (address >= ARM7_WRAM_START && address < IO_REGS_START)
        return *(uint16_t*)&arm7_WRAM[address & 0xFFFF];
    switch (address)
    {
        case 0x04000004:
            return gpu.get_DISPSTAT7();
        case 0x04000006:
            return gpu.get_VCOUNT();
        case 0x040000BA:
            return dma.read_CNT(4);
        case 0x040000C6:
            return dma.read_CNT(5);
        case 0x040000D2:
            return dma.read_CNT(6);
        case 0x040000DE:
            return dma.read_CNT(7);
        case 0x04000100:
            return timers.read_lo(0);
        case 0x04000102:
            return timers.read_hi(0);
        case 0x04000104:
            return timers.read_lo(1);
        case 0x04000106:
            return timers.read_hi(1);
        case 0x04000108:
            return timers.read_lo(2);
        case 0x0400010A:
            return timers.read_hi(2);
        case 0x0400010C:
            return timers.read_lo(3);
        case 0x0400010E:
            return timers.read_hi(3);
        case 0x04000128:
            return SIOCNT;
        case 0x04000130:
            return KEYINPUT.get();
        case 0x04000134:
            return RCNT;
        case 0x04000136:
            return EXTKEYIN.get();
        case 0x04000138:
            return rtc.read();
        case 0x04000180:
            return IPCSYNC_NDS7.read();
        case 0x04000184:
            return fifo7.read_CNT();
        case 0x040001A0:
            return cart.get_AUXSPICNT();
        case 0x040001A2:
            return cart.read_AUXSPIDATA();
        case 0x040001C0:
            return spi.get_SPICNT();
        case 0x040001C2:
            return spi.read_SPIDATA();
        case 0x04000208:
            return int7_reg.IME;
        case 0x04000300:
            return POSTFLG7;
        case 0x04000304:
            return POWCNT2.get();
        case 0x04000500:
            return spu.get_SOUNDCNT();
        case 0x04000504:
            return spu.get_SOUNDBIAS();
        case 0x04000508:
            return spu.get_SNDCAP0() | (spu.get_SNDCAP1() << 8);
        case 0x04001080:
            return 0;
        case 0x04004700:
            return 0;
        /*case 0x04808044:
            return wifi.get_W_RANDOM();
        case 0x0480815C:
            return wifi.get_W_BB_READ();
        case 0x0480815E:
            return wifi.get_W_BB_BUSY();
        case 0x04808180:
            return wifi.get_W_RF_BUSY();*/
    }
    if (address >= 0x04800000 && address < 0x04900000)
        return wifi.read(address);
    if (address >= 0x06000000 && address < 0x07000000)
        return gpu.read_ARM7<uint16_t>(address);
    if (address >= GBA_ROM_START && address < GBA_RAM_START)
        return slot2.read<uint16_t>(address);
    printf("\n(7) Unrecognized halfword read from $%08X", address);
    //exit(2);
    return 0;
}

uint8_t Emulator::arm7_read_byte(uint32_t address)
{
    //TODO: bad hack.
    if (gba_mode)
        return gba_read_byte(address);
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
        return main_RAM[address & MAIN_RAM_MASK];
    if (address >= ARM7_WRAM_START && address < IO_REGS_START)
        return arm7_WRAM[address & ARM7_WRAM_MASK];
    if (address >= SHARED_WRAM_START && address < ARM7_WRAM_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Mirror to ARM7 WRAM
                return arm7_WRAM[address & ARM7_WRAM_MASK];
            case 1: //First half
                return shared_WRAM[address & 0x3FFF];
            case 2: //Second half
                return shared_WRAM[(address & 0x3FFF) + 0x4000];
            case 3: //Entire 32 KB
                return shared_WRAM[address & 0x7FFF];
        }
    }
    switch (address)
    {
        case 0x04000130:
            return KEYINPUT.get() & 0xFF;
        case 0x04000138:
            return rtc.read();
        case 0x040001C2:
            return spi.read_SPIDATA();
        case 0x04000218: //DSi IE2
            return 0;
        case 0x0400021C: //DSi IF2
            return 0;
        case 0x04000241:
            return WRAMCNT & 0x3;
        case 0x04000300:
            return POSTFLG7;
        case 0x04000501:
            return spu.get_SOUNDCNT() >> 8;
        case 0x04000508:
            return spu.get_SNDCAP0();
        case 0x04000509:
            return spu.get_SNDCAP1();
    }
    if (address < 0x4000)
    {
        if (arm7.get_PC() > 0x4000)
            return 0xFF;
        if (address < BIOSPROT && arm7.get_PC() > BIOSPROT)
            return 0xFF;
        return arm7_bios[address];
    }
    if (address >= 0x04000400 && address < 0x04000500)
        return spu.read_channel_byte(address);
    if (address >= GBA_ROM_START && address < GBA_RAM_START)
        return slot2.read<uint8_t>(address);
    printf("\n(7) Unrecognized byte read from $%08X", address);
    //exit(2);
    return 0;
}

void Emulator::arm7_write_word(uint32_t address, uint32_t word)
{
    //TODO: bad hack.
    if (gba_mode)
    {
        gba_write_word(address, word);
        return;
    }
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        *(uint32_t*)&main_RAM[address & MAIN_RAM_MASK] = word;
        return;
    }
    if (address >= SHARED_WRAM_START && address < ARM7_WRAM_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Mirror to ARM7 WRAM
                *(uint32_t*)&arm7_WRAM[address & ARM7_WRAM_MASK] = word;
                return;
            case 1: //First half
                *(uint32_t*)&shared_WRAM[address & 0x3FFF] = word;
                return;
            case 2: //Second half
                *(uint32_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000] = word;
                return;
            case 3: //Entire 32 KB
                *(uint32_t*)&shared_WRAM[address & 0x7FFF] = word;
                return;
        }
    }
    if (address >= ARM7_WRAM_START && address < IO_REGS_START)
    {
        *(uint32_t*)&arm7_WRAM[address & ARM7_WRAM_MASK] = word;
        return;
    }
    switch (address)
    {
        case 0x040000B0:
            dma.write_source(4, word);
            return;
        case 0x040000B4:
            dma.write_dest(4, word);
            return;
        case 0x040000B8:
            dma.write_len_CNT(4, word);
            return;
        case 0x040000BC:
            dma.write_source(5, word);
            return;
        case 0x040000C0:
            dma.write_dest(5, word);
            return;
        case 0x040000C4:
            dma.write_len_CNT(5, word);
            return;
        case 0x040000C8:
            dma.write_source(6, word);
            return;
        case 0x040000CC:
            dma.write_dest(6, word);
            return;
        case 0x040000D0:
            dma.write_len_CNT(6, word);
            return;
        case 0x040000D4:
            dma.write_source(7, word);
            return;
        case 0x040000D8:
            dma.write_dest(7, word);
            return;
        case 0x040000DC:
            dma.write_len_CNT(7, word);
            return;
        case 0x04000100:
            timers.write_lo(word & 0xFFFF, 0);
            timers.write_hi(word >> 16, 0);
            return;
        case 0x04000104:
            timers.write_lo(word & 0xFFFF, 1);
            timers.write_hi(word >> 16, 1);
            return;
        case 0x04000108:
            timers.write_lo(word & 0xFFFF, 2);
            timers.write_hi(word >> 16, 2);
            return;
        case 0x0400010C:
            timers.write_lo(word & 0xFFFF, 3);
            timers.write_hi(word >> 16, 3);
            return;
        case 0x04000120: //Debug SIODATA32
            return;
        case 0x04000128:
            return;
        case 0x04000180:
            IPCSYNC_NDS7.write(word);
            IPCSYNC_NDS9.receive_input(word);

            if (word & (1 << 13) && IPCSYNC_NDS9.IRQ_enable)
                request_interrupt9(INTERRUPT::IPCSYNC);
            return;
        case 0x04000188:
            fifo9.write_queue(word);
            if (fifo9.request_nempty_IRQ)
            {
                request_interrupt9(INTERRUPT::IPC_FIFO_NEMPTY);
                fifo9.request_nempty_IRQ = false;
            }
            return;
        case 0x040001A4:
            cart.set_ROMCTRL(word);
            return;
        case 0x040001B0:
            cart.set_lo_key2_seed0(word);
            return;
        case 0x040001B4:
            cart.set_lo_key2_seed1(word);
            return;
        case 0x04000208:
            int7_reg.IME = word & 0x1;
            return;
        case 0x04000210:
            int7_reg.IE = word;
            return;
        case 0x04000214:
            int7_reg.IF &= ~word;
            return;
        case 0x04000218:
            return;
        case 0x04000308:
            BIOSPROT = word & ~0x1;
            return;
        case 0x04000500:
            spu.set_SOUNDCNT(word & 0xFFFF);
            return;
        case 0x04000510:
            return;
        case 0x04000518:
            return;
    }
    if (address >= 0x04000400 && address < 0x04000500)
    {
        spu.write_channel_word(address, word);
        return;
    }
    if (address >= 0x06000000 && address < 0x07000000)
    {
        gpu.write_ARM7<uint32_t>(address, word);
        return;
    }
    printf("\n(7) Unrecognized word write of $%08X to $%08X", word, address);
    //exit(2);
}

void Emulator::arm7_write_halfword(uint32_t address, uint16_t halfword)
{
    //TODO: bad hack.
    if (gba_mode)
    {
        gba_write_halfword(address, halfword);
        return;
    }
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        *(uint16_t*)&main_RAM[address & MAIN_RAM_MASK] = halfword;
        return;
    }
    if (address >= SHARED_WRAM_START && address < ARM7_WRAM_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Mirror to ARM7 WRAM
                *(uint16_t*)&arm7_WRAM[address & ARM7_WRAM_MASK] = halfword;
                return;
            case 1: //First half
                *(uint16_t*)&shared_WRAM[address & 0x3FFF] = halfword;
                return;
            case 2: //Second half
                *(uint16_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000] = halfword;
                return;
            case 3: //Entire 32 KB
                *(uint16_t*)&shared_WRAM[address & 0x7FFF] = halfword;
                return;
        }
    }
    if (address >= ARM7_WRAM_START && address < IO_REGS_START)
    {
        *(uint16_t*)&arm7_WRAM[address & ARM7_WRAM_MASK] = halfword;
        return;
    }
    switch (address)
    {
        case 0x04000004:
            gpu.set_DISPSTAT7(halfword);
            return;
        case 0x040000BA:
            dma.write_CNT(4, halfword);
            return;
        case 0x040000C6:
            dma.write_CNT(5, halfword);
            return;
        case 0x040000D2:
            dma.write_CNT(6, halfword);
            return;
        case 0x040000DE:
            dma.write_CNT(7, halfword);
            return;
        case 0x04000100:
            timers.write_lo(halfword, 0);
            return;
        case 0x04000102:
            timers.write_hi(halfword, 0);
            return;
        case 0x04000104:
            timers.write_lo(halfword, 1);
            return;
        case 0x04000106:
            timers.write_hi(halfword, 1);
            return;
        case 0x04000108:
            timers.write_lo(halfword, 2);
            return;
        case 0x0400010A:
            timers.write_hi(halfword, 2);
            return;
        case 0x0400010C:
            timers.write_lo(halfword, 3);
            return;
        case 0x0400010E:
            timers.write_hi(halfword, 3);
            return;
        case 0x04000128: //Debug SIOCNT
            SIOCNT = halfword;
            return;
        case 0x04000134: //Debug RCNT
            RCNT = halfword;
            return;
        case 0x04000138:
            rtc.write(halfword, false);
            return;
        case 0x04000180:
            IPCSYNC_NDS7.write(halfword);
            IPCSYNC_NDS9.receive_input(halfword);

            if (halfword & (1 << 13) && IPCSYNC_NDS9.IRQ_enable)
                request_interrupt9(INTERRUPT::IPCSYNC);
            return;
        case 0x04000184:
            fifo7.write_CNT(halfword);
            if (fifo7.request_empty_IRQ)
            {
                request_interrupt7(INTERRUPT::IPC_FIFO_EMPTY);
                fifo7.request_empty_IRQ = false;
            }
            if (fifo7.request_nempty_IRQ)
            {
                request_interrupt7(INTERRUPT::IPC_FIFO_NEMPTY);
                fifo7.request_nempty_IRQ = false;
            }
            return;
        case 0x040001A0:
            cart.set_AUXSPICNT(halfword);
            return;
        case 0x040001A2:
            //printf("\nAUXSPIDATA: $%04X", halfword);
            cart.set_AUXSPIDATA(halfword & 0xFF);
            return;
        case 0x040001B8:
            cart.set_hi_key2_seed0(halfword);
            return;
        case 0x040001BA:
            cart.set_hi_key2_seed1(halfword);
            return;
        case 0x040001C0:
            spi.set_SPICNT(halfword);
            return;
        case 0x040001C2:
            spi.write_SPIDATA(halfword & 0xFF);
            return;
        case 0x04000206:
            //TODO: WIFIWAITCNT
            return;
        case 0x04000208:
            int7_reg.IME = halfword & 0x1;
            return;
        case 0x04000300:
            POSTFLG7 = halfword & 0x1;
            return;
        case 0x04000304:
            POWCNT2.set(halfword);
            return;
        case 0x04000500:
            spu.set_SOUNDCNT(halfword);
            return;
        case 0x04000504:
            spu.set_SOUNDBIAS(halfword);
            return;
        case 0x04000508:
            spu.set_SNDCAP0(halfword & 0xFF);
            spu.set_SNDCAP1(halfword >> 8);
            return;
        case 0x04000514:
            return;
        case 0x0400051C:
            return;
        case 0x04001080:
            //Some sort of debugging port related to the DS-lite firmware, can be ignored
            return;
    }
    if (address >= 0x04800000 && address < 0x04900000)
    {
        wifi.write(address, halfword);
        return;
    }
    if (address >= 0x04000400 && address < 0x04000500)
    {
        spu.write_channel_halfword(address, halfword);
        return;
    }
    if (address >= 0x06000000 && address < 0x07000000)
    {
        gpu.write_ARM7<uint16_t>(address, halfword);
        return;
    }
    printf("\n(7) Unrecognized halfword write of $%04X to $%08X", halfword, address);
    //exit(2);
}

void Emulator::arm7_write_byte(uint32_t address, uint8_t byte)
{
    //TODO: bad hack.
    if (gba_mode)
    {
        gba_write_byte(address, byte);
        return;
    }
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        main_RAM[address & MAIN_RAM_MASK] = byte;
        return;
    }
    if (address >= ARM7_WRAM_START && address < IO_REGS_START)
    {
        arm7_WRAM[address & ARM7_WRAM_MASK] = byte;
        return;
    }
    if (address >= SHARED_WRAM_START && address < ARM7_WRAM_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Mirror to ARM7 WRAM
                arm7_WRAM[address & ARM7_WRAM_MASK] = byte;
                return;
            case 1: //First half
                shared_WRAM[address & 0x3FFF] = byte;
                return;
            case 2: //Second half
                shared_WRAM[(address & 0x3FFF) + 0x4000] = byte;
                return;
            case 3: //Entire 32 KB
                shared_WRAM[address & 0x7FFF] = byte;
                return;
        }
    }
    switch (address)
    {
        case 0x04000138:
            rtc.write(byte, true);
            return;
        case 0x040001A1:
            cart.set_hi_AUXSPICNT(byte);
            return;
        case 0x040001A8:
        case 0x040001A9:
        case 0x040001AA:
        case 0x040001AB:
        case 0x040001AC:
        case 0x040001AD:
        case 0x040001AE:
        case 0x040001AF:
            cart.receive_command(byte, address - 0x040001A8);
            return;
        case 0x040001C2:
            spi.write_SPIDATA(byte);
            return;
        case 0x04000208:
            int7_reg.IME = byte & 0x1;
            return;
        case 0x04000300:
            POSTFLG7 = byte & 0x1;
            return;
        case 0x04000301:
            switch (byte)
            {
                case 0x40:
                    start_gba_mode(true);
                    break;
                case 0x80:
                    arm7.halt();
                    /*printf("\nHalted ARM7");
                    printf("\nIE: $%08X", int7_reg.IE);
                    printf("\nIF: $%08X", int9_reg.IF);*/
                    break;
                default:
                    throw "Unrecognized HALTCNT state for ARM7";
            }
            return;
        case 0x04000500:
            spu.set_SOUNDCNT_lo(byte);
            return;
        case 0x04000501:
            spu.set_SOUNDCNT_hi(byte);
            return;
        case 0x04000508:
            spu.set_SNDCAP0(byte);
            return;
        case 0x04000509:
            spu.set_SNDCAP1(byte);
            return;
    }
    if (address >= 0x04000400 && address < 0x04000500)
    {
        spu.write_channel_byte(address, byte);
        return;
    }
    if (address >= 0x06000000 && address < 0x07000000)
    {
        gpu.write_ARM7<uint8_t>(address, byte);
        return;
    }
    if (address < 0x4000) //Ignore BIOS writes
        return;
    printf("\n(7) Unrecognized byte write of $%02X to $%08X", byte, address);
    //exit(2);
}
