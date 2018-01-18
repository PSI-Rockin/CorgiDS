/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "config.hpp"
#include "emulator.hpp"

uint32_t Emulator::arm9_read_word(uint32_t address)
{
    if (address >= 0xFFFF0000)
        return *(uint32_t*)&arm9_bios[address - 0xFFFF0000];
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
        return *(uint32_t*)&main_RAM[address & MAIN_RAM_MASK];
    if ((address >= IO_REGS_START && address < GBA_ROM_START) || address == 0x021F6404)
    {
        if (Config::test)
            printf("\nRead32 $%08X", address);
    }
    if (address >= SHARED_WRAM_START && address < IO_REGS_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Entire 32 KB
                return *(uint32_t*)&shared_WRAM[address & 0x7FFF];
            case 1: //Second half
                return *(uint32_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000];
            case 2: //First half
                return *(uint32_t*)&shared_WRAM[address & 0x3FFF];
            case 3: //Undefined memory
                return 0;
        }
    }
    switch (address)
    {
        case 0x04000000:
            return gpu.get_DISPCNT_A();
        case 0x04000010:
            return gpu.get_BGHOFS_A(0) | (gpu.get_BGVOFS_A(0) << 16);
        case 0x04000014:
            return gpu.get_BGHOFS_A(1) | (gpu.get_BGVOFS_A(1) << 16);
        case 0x04000018:
            return gpu.get_BGHOFS_A(2) | (gpu.get_BGVOFS_A(2) << 16);
        case 0x0400001C:
            return gpu.get_BGHOFS_A(3) | (gpu.get_BGVOFS_A(3) << 16);
        case 0x04000064:
            return gpu.get_DISPCAPCNT();
        case 0x040000B0:
            return dma.read_source(0);
        case 0x040000B8:
            return dma.read_len(0) | (dma.read_CNT(0) << 16);
        case 0x040000C4:
            return dma.read_len(1) | (dma.read_CNT(1) << 16);
        case 0x040000D0:
            return dma.read_len(2) | (dma.read_CNT(2) << 16);
        case 0x040000DC:
            return dma.read_len(3) | (dma.read_CNT(3) << 16);
        case 0x040000E0:
            return DMAFILL[0];
        case 0x040000E4:
            return DMAFILL[1];
        case 0x040000E8:
            return DMAFILL[2];
        case 0x040000EC:
            return DMAFILL[3];
        case 0x04000180:
            return IPCSYNC_NDS9.read();
        case 0x040001A4:
            return cart.get_ROMCTRL();
        case 0x04000208:
            return int9_reg.IME;
        case 0x04000210:
            return int9_reg.IE;
        case 0x04000214:
            return int9_reg.IF;
        case 0x04000240:
            return gpu.get_VRAMCNT_A();
        case 0x04000290:
            return DIV_NUMER & 0xFFFFFFFF;
        case 0x04000294:
            return DIV_NUMER >> 32;
        case 0x04000298:
            return DIV_DENOM & 0xFFFFFFFF;
        case 0x0400029C:
            return DIV_DENOM >> 32;
        case 0x040002A0:
            return DIV_RESULT & 0xFFFFFFFF;
        case 0x040002A4:
            return DIV_RESULT >> 32;
        case 0x040002A8:
            return DIV_REMRESULT & 0xFFFFFFFF;
        case 0x040002AC:
            return DIV_REMRESULT >> 32;
        case 0x040002B4:
            return SQRT_RESULT;
        case 0x040002B8:
            return SQRT_PARAM & 0xFFFFFFFF;
        case 0x040002BC:
            return SQRT_PARAM >> 32;
        case 0x040004A4:
            return 0; //POLYGON_ATTR is unreadable but games will spam the console without this
        case 0x04000600:
            return gpu.get_GXSTAT();
        case 0x04000604:
            return gpu.get_poly_count() | (gpu.get_vert_count() << 16);
        case 0x04001000:
            return gpu.get_DISPCNT_B();
        case 0x04001010:
            return gpu.get_BGHOFS_B(0) | (gpu.get_BGVOFS_B(0) << 16);
        case 0x04001014:
            return gpu.get_BGHOFS_B(1) | (gpu.get_BGVOFS_B(1) << 16);
        case 0x04001018:
            return gpu.get_BGHOFS_B(2) | (gpu.get_BGVOFS_B(2) << 16);
        case 0x0400101C:
            return gpu.get_BGHOFS_B(3) | (gpu.get_BGVOFS_B(3) << 16);
        case 0x04004000:
            return 0;
        case 0x04004008:
            return 0;
        case 0x04100000:
        {
            uint32_t word = fifo7.read_queue();
            if (fifo7.request_empty_IRQ)
            {
                request_interrupt7(INTERRUPT::IPC_FIFO_EMPTY);
                fifo7.request_empty_IRQ = false;
            }
            return word;
        }
        case 0x04100010:
            return cart.get_output();
    }
    if (address >= PALETTE_START && address < VRAM_BGA_START)
    {
        if ((address & 0x7FF) < 0x400)
            return gpu.read_palette_A(address) | (gpu.read_palette_A(address + 2) << 16);
        else
            return gpu.read_palette_B(address) | (gpu.read_palette_B(address + 2) << 16);
    }
    if (address >= 0x04000640 && address < 0x04000680)
        return gpu.read_clip_mtx(address);
    if (address >= 0x04000680 && address < 0x040006A4)
        return gpu.read_vec_mtx(address);
    if (address >= VRAM_BGA_START && address < VRAM_BGB_START)
        return gpu.read_bga<uint32_t>(address);
    if (address >= VRAM_BGB_START && address < VRAM_OBJA_START)
        return gpu.read_bgb<uint32_t>(address);
    if (address >= VRAM_OBJA_START && address < VRAM_OBJB_START)
        return gpu.read_obja<uint32_t>(address);
    if (address >= VRAM_OBJB_START && address < VRAM_LCDC_A)
        return gpu.read_objb<uint32_t>(address);
    if (address >= VRAM_LCDC_A && address < VRAM_LCDC_END)
        return gpu.read_lcdc<uint32_t>(address);
    if (address >= OAM_START && address < GBA_ROM_START)
        return gpu.read_OAM<uint32_t>(address);
    if (address >= GBA_ROM_START && address < GBA_RAM_START)
        return slot2.read<uint32_t>(address);
    //Ignore reads from NULL
    printf("\n(9) Unrecognized word read from $%08X", address);
    //exit(1);
    return 0;
}

uint16_t Emulator::arm9_read_halfword(uint32_t address)
{
    if (address >= 0xFFFF0000)
        return *(uint16_t*)&arm9_bios[address - 0xFFFF0000];
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        return *(uint16_t*)&main_RAM[address & MAIN_RAM_MASK];
    }
    if (address >= IO_REGS_START && address < GBA_ROM_START)
    {
        if (Config::test)
            printf("\nRead16 $%08X", address);
    }
    if (address >= SHARED_WRAM_START && address < IO_REGS_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Entire 32 KB
                return *(uint16_t*)&shared_WRAM[address & 0x7FFF];
            case 1: //Second half
                return *(uint16_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000];
            case 2: //First half
                return *(uint16_t*)&shared_WRAM[address & 0x3FFF];
            case 3: //Undefined memory
                return 0;
        }
    }
    switch (address)
    {
        case 0x04000000:
            return gpu.get_DISPCNT_A() & 0xFFFF;
        case 0x04000004:
            return gpu.get_DISPSTAT9();
        case 0x04000006:
            return gpu.get_VCOUNT();
        case 0x04000008:
            return gpu.get_BGCNT_A(0);
        case 0x0400000A:
            return gpu.get_BGCNT_A(1);
        case 0x0400000C:
            return gpu.get_BGCNT_A(2);
        case 0x0400000E:
            return gpu.get_BGCNT_A(3);
        case 0x04000044:
            return gpu.get_WIN0V_A();
        case 0x04000046:
            return gpu.get_WIN1V_A();
        case 0x04000048:
            return gpu.get_WININ_A();
        case 0x0400004A:
            return gpu.get_WINOUT_A();
        case 0x04000050:
            return gpu.get_BLDCNT_A();
        case 0x04000052:
            return gpu.get_BLDALPHA_A();
        case 0x04000060:
            return gpu.get_DISP3DCNT();
        case 0x0400006C:
            return gpu.get_MASTER_BRIGHT_A();
        case 0x040000BA:
            return dma.read_CNT(0);
        case 0x040000C6:
            return dma.read_CNT(1);
        case 0x040000D2:
            return dma.read_CNT(2);
        case 0x040000DE:
            return dma.read_CNT(3);
        case 0x040000E0:
            return DMAFILL[0] & 0xFFFF;
        case 0x040000E4:
            return DMAFILL[1] & 0xFFFF;
        case 0x040000E8:
            return DMAFILL[2] & 0xFFFF;
        case 0x040000EC:
            return DMAFILL[3] & 0xFFFF;
        case 0x04000100:
            return timers.read_lo(4);
        case 0x04000104:
            return timers.read_lo(5);
        case 0x04000108:
            return timers.read_lo(6);
        case 0x0400010C:
            return timers.read_lo(7);
        case 0x04000130:
            return KEYINPUT.get();
        case 0x04000180:
            return IPCSYNC_NDS9.read();
        case 0x04000184:
            return fifo9.read_CNT();
        case 0x040001A0:
            return cart.get_AUXSPICNT();
        case 0x04000204:
            return EXMEMCNT;
        case 0x04000208:
            return int9_reg.IME;
        case 0x04000210:
            return int9_reg.IE & 0xFFFF;
        case 0x04000212:
            return int9_reg.IE >> 16;
        case 0x04000214:
            return int9_reg.IF & 0xFFFF;
        case 0x04000216:
            return int9_reg.IF >> 16;
        case 0x04000280:
            return DIVCNT;
        case 0x040002B0:
            return SQRTCNT;
        case 0x04000300:
            return POSTFLG9;
        case 0x04000304:
            return gpu.get_POWCNT1();
        case 0x04000604:
            return gpu.get_poly_count();
        case 0x04000606:
            return gpu.get_vert_count();
        case 0x04001000:
            return gpu.get_DISPCNT_B() & 0xFFFF;
        case 0x04001008:
            return gpu.get_BGCNT_B(0);
        case 0x0400100A:
            return gpu.get_BGCNT_B(1);
        case 0x0400100C:
            return gpu.get_BGCNT_B(2);
        case 0x0400100E:
            return gpu.get_BGCNT_B(3);
        case 0x04001044:
            return gpu.get_WIN0V_B();
        case 0x04001046:
            return gpu.get_WIN1V_B();
        case 0x04001048:
            return gpu.get_WININ_B();
        case 0x0400104A:
            return gpu.get_WINOUT_B();
        case 0x04001050:
            return gpu.get_BLDCNT_B();
        case 0x04001052:
            return gpu.get_BLDALPHA_B();
        case 0x0400106C:
            return gpu.get_MASTER_BRIGHT_B();
    }
    if (address >= 0x04000630 && address < 0x04000636)
        return gpu.read_vec_test(address);
    if (address >= PALETTE_START && address < VRAM_BGA_START)
    {
        if ((address & 0x7FF) < 0x400)
            return gpu.read_palette_A(address);
        else
            return gpu.read_palette_B(address);
    }
    if (address >= VRAM_BGA_START && address < VRAM_BGB_START)
        return gpu.read_bga<uint16_t>(address);
    if (address >= VRAM_BGB_START && address < VRAM_OBJA_START)
        return gpu.read_bgb<uint16_t>(address);
    if (address >= VRAM_OBJA_START && address < VRAM_OBJB_START)
        return gpu.read_obja<uint16_t>(address);
    if (address >= VRAM_OBJB_START && address < VRAM_LCDC_A)
        return gpu.read_objb<uint16_t>(address);
    if (address >= VRAM_LCDC_A && address < VRAM_LCDC_END)
        return gpu.read_lcdc<uint16_t>(address);
    if (address >= GBA_ROM_START && address < GBA_RAM_START)
        return slot2.read<uint16_t>(address);
    printf("\n(9) Unrecognized halfword read from $%08X", address);
    return 0;
    //exit(1);
}

uint8_t Emulator::arm9_read_byte(uint32_t address)
{
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        return main_RAM[address & MAIN_RAM_MASK];
    }
    if (address >= SHARED_WRAM_START && address < IO_REGS_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Entire 32 KB
                return shared_WRAM[address & 0x7FFF];
            case 1: //Second half
                return shared_WRAM[(address & 0x3FFF) + 0x4000];
            case 2: //First half
                return shared_WRAM[address & 0x3FFF];
            case 3: //Undefined memory
                return 0;
        }
    }
    if (address >= IO_REGS_START && address < GBA_ROM_START)
    {
        if (Config::test)
            printf("\nRead8 $%08X", address);
    }
    switch (address)
    {
        case 0x040001A2:
            return cart.read_AUXSPIDATA();
        case 0x040001A8:
        case 0x040001A9:
        case 0x040001AA:
        case 0x040001AB:
        case 0x040001AC:
        case 0x040001AD:
        case 0x040001AE:
        case 0x040001AF:
            return cart.read_command(address & 0x7);
        case 0x04000208:
            return int9_reg.IME;
        case 0x04000240:
            return gpu.get_VRAMCNT_A();
        case 0x04000241:
            return gpu.get_VRAMCNT_B();
        case 0x04000247:
            return WRAMCNT & 0x3;
        case 0x04000300:
            return POSTFLG9;
        case 0x04004000:
            return 0;
    }
    if (address >= PALETTE_START && address < VRAM_BGA_START)
    {
        if ((address & 0x7FF) < 0x400)
            return gpu.read_palette_A(address) & 0xFF;
        else
            return gpu.read_palette_B(address) & 0xFF;
    }
    if (address >= VRAM_BGA_START && address < VRAM_BGB_START)
        return gpu.read_bga<uint8_t>(address);
    if (address >= VRAM_BGB_START && address < VRAM_OBJA_START)
        return gpu.read_bgb<uint8_t>(address);
    if (address >= VRAM_OBJA_START && address < VRAM_OBJB_START)
        return gpu.read_obja<uint8_t>(address);
    if (address >= VRAM_OBJB_START && address < VRAM_LCDC_A)
        return gpu.read_objb<uint8_t>(address);
    if (address >= VRAM_LCDC_A && address < OAM_START)
        return gpu.read_lcdc<uint8_t>(address);
    if (address >= OAM_START && address < GBA_ROM_START)
        return gpu.read_OAM<uint8_t>(address);
    if (address >= GBA_ROM_START && address < GBA_RAM_START)
        return slot2.read<uint8_t>(address);
    printf("\n(9) Unrecognized byte read from $%08X", address);
    //exit(1);
    return 0;
}

void Emulator::arm9_write_word(uint32_t address, uint32_t word)
{
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        *(uint32_t*)&main_RAM[address & MAIN_RAM_MASK] = word;
        return;
    }
    if (address >= IO_REGS_START && address < GBA_ROM_START)
    {
        if (Config::test)
            printf("\nWrite32: $%08X $%08X", address, word);
    }
    if (address >= SHARED_WRAM_START && address < IO_REGS_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Entire 32 KB
                *(uint32_t*)&shared_WRAM[address & 0x7FFF] = word;
                return;
            case 1: //Second half
                *(uint32_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000] = word;
                return;
            case 2: //First half
                *(uint32_t*)&shared_WRAM[address & 0x3FFF] = word;
                return;
            case 3: //Undefined memory
                return;
        }
    }
    switch (address)
    {
        case 0x04000000:
            gpu.set_DISPCNT_A(word);
            return;
        case 0x04000004:
            gpu.set_DISPSTAT9(word & 0xFFFF);
            return;
        case 0x04000008:
            gpu.set_BGCNT_A(word & 0xFFFF, 0);
            gpu.set_BGCNT_A(word >> 16, 1);
            return;
        case 0x0400000C:
            gpu.set_BGCNT_A(word & 0xFFFF, 2);
            gpu.set_BGCNT_A(word >> 16, 3);
            return;
        case 0x04000010:
            gpu.set_BGHOFS_A(word & 0xFFFF, 0);
            gpu.set_BGVOFS_A(word >> 16, 0);
            return;
        case 0x04000014:
            gpu.set_BGHOFS_A(word & 0xFFFF, 1);
            gpu.set_BGVOFS_A(word >> 16, 1);
            return;
        case 0x04000018:
            gpu.set_BGHOFS_A(word & 0xFFFF, 2);
            gpu.set_BGVOFS_A(word >> 16, 2);
            return;
        case 0x0400001C:
            gpu.set_BGHOFS_A(word & 0xFFFF, 3);
            gpu.set_BGVOFS_A(word >> 16, 3);
            return;
        case 0x04000020:
            gpu.set_BG2P_A(word & 0xFFFF, 0);
            gpu.set_BG2P_A(word >> 16, 1);
            return;
        case 0x04000024:
            gpu.set_BG2P_A(word & 0xFFFF, 2);
            gpu.set_BG2P_A(word >> 16, 3);
            return;
        case 0x04000028:
            gpu.set_BG2X_A(word);
            return;
        case 0x0400002C:
            gpu.set_BG2Y_A(word);
            return;
        case 0x04000030:
            gpu.set_BG3P_A(word & 0xFFFF, 0);
            gpu.set_BG3P_A(word >> 16, 1);
            return;
        case 0x04000034:
            gpu.set_BG3P_A(word & 0xFFFF, 2);
            gpu.set_BG3P_A(word >> 16, 3);
            return;
        case 0x04000038:
            gpu.set_BG3X_A(word);
            return;
        case 0x0400003C:
            gpu.set_BG3Y_A(word);
            return;
        case 0x04000040:
            gpu.set_WIN0H_A(word & 0xFFFF);
            gpu.set_WIN1H_A(word >> 16);
            return;
        case 0x04000044:
            gpu.set_WIN0V_A(word & 0xFFFF);
            gpu.set_WIN1V_A(word >> 16);
            return;
        case 0x04000048:
            gpu.set_WININ_A(word & 0xFFFF);
            gpu.set_WINOUT_A(word >> 16);
            return;
        case 0x0400004C:
            gpu.set_MOSAIC_A(word & 0xFFFF);
            return;
        case 0x04000050:
            gpu.set_BLDCNT_A(word & 0xFFFF);
            gpu.set_BLDALPHA_A(word >> 16);
            return;
        case 0x04000054:
            gpu.set_BLDY_A(word & 0x1F);
            return;
        case 0x04000058:
        case 0x0400005C:
        case 0x04000060:
            return;
        case 0x04000064:
            gpu.set_DISPCAPCNT(word);
            return;
        case 0x04000068:
            return;
        case 0x0400006C:
            gpu.set_MASTER_BRIGHT_A(word & 0xFFFF);
            return;
        case 0x04000070:
            return;
        case 0x040000B0:
            dma.write_source(0, word);
            return;
        case 0x040000B4:
            dma.write_dest(0, word);
            return;
        case 0x040000B8:
            dma.write_len_CNT(0, word);
            return;
        case 0x040000BC:
            dma.write_source(1, word);
            return;
        case 0x040000C0:
            dma.write_dest(1, word);
            return;
        case 0x040000C4:
            dma.write_len_CNT(1, word);
            return;
        case 0x040000C8:
            dma.write_source(2, word);
            return;
        case 0x040000CC:
            dma.write_dest(2, word);
            return;
        case 0x040000D0:
            dma.write_len_CNT(2, word);
            return;
        case 0x040000D4:
            dma.write_source(3, word);
            return;
        case 0x040000D8:
            dma.write_dest(3, word);
            return;
        case 0x040000DC:
            dma.write_len_CNT(3, word);
            return;
        case 0x040000E0:
            DMAFILL[0] = word;
            return;
        case 0x040000E4:
            DMAFILL[1] = word;
            return;
        case 0x040000E8:
            DMAFILL[2] = word;
            return;
        case 0x040000EC:
            DMAFILL[3] = word;
            return;
        case 0x04000180:
            IPCSYNC_NDS9.write(word);
            IPCSYNC_NDS7.receive_input(word);

            if (word & (1 << 13) && IPCSYNC_NDS7.IRQ_enable)
                request_interrupt7(INTERRUPT::IPCSYNC);
            return;
        case 0x04000188:
            fifo7.write_queue(word);
            if (fifo7.request_nempty_IRQ)
            {
                request_interrupt7(INTERRUPT::IPC_FIFO_NEMPTY);
                fifo7.request_nempty_IRQ = false;
            }
            return;
        case 0x040001A0:
            cart.set_AUXSPICNT(word & 0xFFFF);
            cart.set_AUXSPIDATA((word >> 16) & 0xFF);
            return;
        case 0x040001A4:
            cart.set_ROMCTRL(word);
            return;
        case 0x040001A8:
            cart.receive_command((word >> 24), 3);
            cart.receive_command((word >> 16) & 0xFF, 2);
            cart.receive_command((word >> 8) & 0xFF, 1);
            cart.receive_command(word & 0xFF, 0);
            return;
        case 0x040001AC:
            cart.receive_command((word >> 24), 7);
            cart.receive_command((word >> 16) & 0xFF, 6);
            cart.receive_command((word >> 8) & 0xFF, 5);
            cart.receive_command(word & 0xFF, 4);
            return;
        case 0x04000208:
            int9_reg.IME = word & 0x1;
            return;
        case 0x04000210:
            int9_reg.IE = word;
            return;
        case 0x04000214:
            int9_reg.IF &= ~word;
            gpu.check_GXFIFO_IRQ(); //The IRQ is continuously set to true as long as the condition is true
            return;
        case 0x04000240:
            gpu.set_VRAMCNT_A(word & 0xFF);
            gpu.set_VRAMCNT_B((word >> 8) & 0xFF);
            gpu.set_VRAMCNT_C((word >> 16) & 0xFF);
            gpu.set_VRAMCNT_D(word >> 24);
            return;
        case 0x04000290:
            DIV_NUMER &= ~0xFFFFFFFF;
            DIV_NUMER |= word;
            start_division();
            return;
        case 0x04000294:
            DIV_NUMER &= 0xFFFFFFFF;
            DIV_NUMER |= (static_cast<uint64_t>(word) << 32);
            start_division();
            return;
        case 0x04000298:
            DIV_DENOM &= ~0xFFFFFFFF;
            DIV_DENOM |= word;
            start_division();
            return;
        case 0x0400029C:
            DIV_DENOM &= 0xFFFFFFFF;
            DIV_DENOM |= (static_cast<uint64_t>(word) << 32);
            start_division();
            return;
        case 0x040002B8:
            SQRT_PARAM &= ~0xFFFFFFFF;
            SQRT_PARAM |= word;
            start_sqrt();
            return;
        case 0x040002BC:
            SQRT_PARAM &= 0xFFFFFFFF;
            SQRT_PARAM |= (static_cast<uint64_t>(word) << 32);
            start_sqrt();
            return;
        case 0x04000304:
            gpu.set_POWCNT1(word & 0xFFFF);
            return;
        case 0x04000350:
            gpu.set_CLEAR_COLOR(word);
            return;
        case 0x04000358:
            //TODO: FOG_COLOR
            gpu.set_FOG_COLOR(word);
            return;
        case 0x04000600:
            gpu.set_GXSTAT(word);
            return;
        case 0x04001000:
            gpu.set_DISPCNT_B(word);
            return;
        case 0x04001004:
            return;
        case 0x04001008:
            gpu.set_BGCNT_B(word & 0xFFFF, 0);
            gpu.set_BGCNT_B(word >> 16, 1);
            return;
        case 0x0400100C:
            gpu.set_BGCNT_B(word & 0xFFFF, 2);
            gpu.set_BGCNT_B(word >> 16, 3);
            return;
        case 0x04001010:
            gpu.set_BGHOFS_B(word & 0xFFFF, 0);
            gpu.set_BGVOFS_B(word >> 16, 0);
            return;
        case 0x04001014:
            gpu.set_BGHOFS_B(word & 0xFFFF, 1);
            gpu.set_BGVOFS_B(word >> 16, 1);
            return;
        case 0x04001018:
            gpu.set_BGHOFS_B(word & 0xFFFF, 2);
            gpu.set_BGVOFS_B(word >> 16, 2);
            return;
        case 0x0400101C:
            gpu.set_BGHOFS_B(word & 0xFFFF, 3);
            gpu.set_BGVOFS_B(word >> 16, 3);
            return;
        case 0x04001020:
            gpu.set_BG2P_B(word & 0xFFFF, 0);
            gpu.set_BG2P_B(word >> 16, 1);
            return;
        case 0x04001024:
            gpu.set_BG2P_B(word & 0xFFFF, 2);
            gpu.set_BG2P_B(word >> 16, 3);
            return;
        case 0x04001028:
            gpu.set_BG2X_B(word);
            return;
        case 0x0400102C:
            gpu.set_BG2Y_B(word);
            return;
        case 0x04001030:
            gpu.set_BG3P_B(word & 0xFFFF, 0);
            gpu.set_BG3P_B(word >> 16, 1);
            return;
        case 0x04001034:
            gpu.set_BG3P_B(word & 0xFFFF, 2);
            gpu.set_BG3P_B(word >> 16, 3);
            return;
        case 0x04001038:
            gpu.set_BG3X_B(word);
            return;
        case 0x0400103C:
            gpu.set_BG3Y_B(word);
            return;
        case 0x04001040:
            gpu.set_WIN0H_B(word & 0xFFFF);
            gpu.set_WIN1H_B(word >> 16);
            return;
        case 0x04001044:
            gpu.set_WIN0V_B(word & 0xFFFF);
            gpu.set_WIN1V_B(word >> 16);
            return;
        case 0x04001048:
            gpu.set_WININ_B(word & 0xFFFF);
            gpu.set_WINOUT_B(word >> 16);
            return;
        case 0x0400104C:
            gpu.set_MOSAIC_B(word & 0xFFFF);
            return;
        case 0x04001050:
            gpu.set_BLDCNT_B(word & 0xFFFF);
            gpu.set_BLDALPHA_B(word >> 16);
            return;
        case 0x04001054:
            gpu.set_BLDY_B(word & 0x1F);
            return;
        case 0x04001058:
        case 0x0400105C:
        case 0x04001060:
        case 0x04001064:
        case 0x04001068:
            return;
        case 0x0400106C:
            gpu.set_MASTER_BRIGHT_B(word & 0xFFFF);
            return;
        case 0x04001070:
            return;
    }
    if (address >= 0x04000330 && address < 0x04000340)
    {
        gpu.set_EDGE_COLOR(address, word & 0xFFFF);
        gpu.set_EDGE_COLOR(address + 2, word >> 16);
        return;
    }
    if (address >= 0x04000360 && address < 0x04000380)
    {
        for (int i = 0; i < 4; i++)
            gpu.set_FOG_TABLE(address + i, (word >> (i * 8)) & 0xFF);
        return;
    }
    if (address >= 0x04000380 && address < 0x040003C0)
    {
        gpu.set_TOON_TABLE((address & 0x3F) >> 1, word & 0xFFFF);
        gpu.set_TOON_TABLE(((address + 2) & 0x3F) >> 1, word >> 16);
        return;
    }
    if (address >= 0x04000400 && address < 0x04000440)
    {
        gpu.write_GXFIFO(word);
        return;
    }
    if (address >= 0x04000440 && address < 0x040005CC)
    {
        gpu.write_FIFO_direct(address, word);
        return;
    }
    if (address >= PALETTE_START && address < VRAM_BGA_START)
    {
        if ((address & 0x7FF) < 0x400)
        {
            gpu.write_palette_A(address, word & 0xFFFF);
            gpu.write_palette_A(address + 2, word >> 16);
        }
        else
        {
            gpu.write_palette_B(address, word & 0xFFFF);
            gpu.write_palette_B(address + 2, word >> 16);
        }
        return;
    }

    if (address >= VRAM_LCDC_A && address < OAM_START)
    {
        gpu.write_lcdc(address, word & 0xFFFF);
        gpu.write_lcdc(address + 2, word >> 16);
        return;
    }

    if (address >= VRAM_BGA_START && address < VRAM_BGB_START)
    {
        gpu.write_bga(address, word & 0xFFFF);
        gpu.write_bga(address + 2, word >> 16);
        return;
    }

    if (address >= VRAM_BGB_START && address < VRAM_OBJA_START)
    {
        gpu.write_bgb(address, word & 0xFFFF);
        gpu.write_bgb(address + 2, word >> 16);
        return;
    }

    if (address >= VRAM_OBJA_START && address < VRAM_OBJB_START)
    {
        gpu.write_obja(address, word & 0xFFFF);
        gpu.write_obja(address + 2, word >> 16);
        return;
    }

    if (address >= VRAM_OBJB_START && address < VRAM_LCDC_A)
    {
        gpu.write_objb(address, word & 0xFFFF);
        gpu.write_objb(address + 2, word >> 16);
        return;
    }

    if (address >= OAM_START && address < GBA_ROM_START)
    {
        gpu.write_OAM(address, word & 0xFFFF);
        gpu.write_OAM(address + 2, word >> 16);
        return;
    }
    printf("\n(9) Unrecognized word write of $%08X to $%08X", word, address);
    //exit(1);
}

void Emulator::arm9_write_halfword(uint32_t address, uint16_t halfword)
{
    if (address >= IO_REGS_START && address < GBA_ROM_START)
    {
        if (Config::test)
            printf("\nWrite16: $%08X $%04X", address, halfword);
    }
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        *(uint16_t*)&main_RAM[address & MAIN_RAM_MASK] = halfword;
        return;
    }
    if (address >= PALETTE_START && address < VRAM_BGA_START)
    {
        if ((address & 0x7FF) < 0x400)
            gpu.write_palette_A(address, halfword);
        else
            gpu.write_palette_B(address, halfword);
        return;
    }
    if (address >= VRAM_LCDC_A && address < OAM_START)
    {
        gpu.write_lcdc(address, halfword);
        return;
    }
    if (address >= VRAM_BGA_START && address < VRAM_BGB_START)
    {
        gpu.write_bga(address, halfword);
        return;
    }
    if (address >= VRAM_BGB_START && address < VRAM_OBJA_START)
    {
        gpu.write_bgb(address, halfword);
        return;
    }
    if (address >= VRAM_OBJA_START && address < VRAM_OBJB_START)
    {
        gpu.write_obja(address, halfword);
        return;
    }
    if (address >= VRAM_OBJB_START && address < VRAM_LCDC_A)
    {
        gpu.write_objb(address, halfword);
        return;
    }
    if (address >= OAM_START && address < GBA_ROM_START)
    {
        gpu.write_OAM(address, halfword);
        return;
    }

    if (address >= SHARED_WRAM_START && address < IO_REGS_START)
    {
        switch (WRAMCNT)
        {
            case 0: //Entire 32 KB
                *(uint16_t*)&shared_WRAM[address & 0x7FFF] = halfword;
                return;
            case 1: //Second half
                *(uint16_t*)&shared_WRAM[(address & 0x3FFF) + 0x4000] = halfword;
                return;
            case 2: //First half
                *(uint16_t*)&shared_WRAM[address & 0x3FFF] = halfword;
                return;
            case 3: //Undefined memory
                return;
        }
    }
    switch (address)
    {
        case 0x04000000:
            gpu.set_DISPCNT_A_lo(halfword);
            return;
        case 0x04000004:
            gpu.set_DISPSTAT9(halfword);
            return;
        case 0x04000008:
            gpu.set_BGCNT_A(halfword, 0);
            return;
        case 0x0400000A:
            gpu.set_BGCNT_A(halfword, 1);
            return;
        case 0x0400000C:
            gpu.set_BGCNT_A(halfword, 2);
            return;
        case 0x0400000E:
            gpu.set_BGCNT_A(halfword, 3);
            return;
        case 0x04000010:
            gpu.set_BGHOFS_A(halfword, 0);
            return;
        case 0x04000012:
            gpu.set_BGVOFS_A(halfword, 0);
            return;
        case 0x04000014:
            gpu.set_BGHOFS_A(halfword, 1);
            return;
        case 0x04000016:
            gpu.set_BGVOFS_A(halfword, 1);
            return;
        case 0x04000018:
            gpu.set_BGHOFS_A(halfword, 2);
            return;
        case 0x0400001A:
            gpu.set_BGVOFS_A(halfword, 2);
            return;
        case 0x0400001C:
            gpu.set_BGHOFS_A(halfword, 3);
            return;
        case 0x0400001E:
            gpu.set_BGVOFS_A(halfword, 3);
            return;
        case 0x04000020:
            gpu.set_BG2P_A(halfword, 0);
            return;
        case 0x04000022:
            gpu.set_BG2P_A(halfword, 1);
            return;
        case 0x04000024:
            gpu.set_BG2P_A(halfword, 2);
            return;
        case 0x04000026:
            gpu.set_BG2P_A(halfword, 3);
            return;
        case 0x04000030:
            gpu.set_BG3P_A(halfword, 0);
            return;
        case 0x04000032:
            gpu.set_BG3P_A(halfword, 1);
            return;
        case 0x04000034:
            gpu.set_BG3P_A(halfword, 2);
            return;
        case 0x04000036:
            gpu.set_BG3P_A(halfword, 3);
            return;
        case 0x04000040:
            gpu.set_WIN0H_A(halfword);
            return;
        case 0x04000042:
            gpu.set_WIN1H_A(halfword);
            return;
        case 0x04000044:
            gpu.set_WIN0V_A(halfword);
            return;
        case 0x04000046:
            gpu.set_WIN1V_A(halfword);
            return;
        case 0x04000048:
            gpu.set_WININ_A(halfword);
            return;
        case 0x0400004A:
            gpu.set_WINOUT_A(halfword);
            return;
        case 0x0400004C:
            gpu.set_MOSAIC_A(halfword);
            return;
        case 0x04000050:
            gpu.set_BLDCNT_A(halfword);
            return;
        case 0x04000052:
            gpu.set_BLDALPHA_A(halfword);
            return;
        case 0x04000054:
            gpu.set_BLDY_A(halfword & 0x1F);
            return;
        case 0x04000060:
            gpu.set_DISP3DCNT(halfword);
            return;
        case 0x0400006C:
            gpu.set_MASTER_BRIGHT_A(halfword);
            return;
        case 0x040000BA:
            dma.write_CNT(0, halfword);
            return;
        case 0x040000C6:
            dma.write_CNT(1, halfword);
            return;
        case 0x040000D0:
            dma.write_len(2, halfword);
            return;
        case 0x040000D2:
            dma.write_CNT(2, halfword);
            return;
        case 0x040000DE:
            dma.write_CNT(3, halfword);
            return;
        case 0x04000100:
            timers.write_lo(halfword, 4);
            return;
        case 0x04000102:
            timers.write_hi(halfword, 4);
            return;
        case 0x04000104:
            timers.write_lo(halfword, 5);
            return;
        case 0x04000106:
            timers.write_hi(halfword, 5);
            return;
        case 0x04000108:
            timers.write_lo(halfword, 6);
            return;
        case 0x0400010A:
            timers.write_hi(halfword, 6);
            return;
        case 0x0400010C:
            timers.write_lo(halfword, 7);
            return;
        case 0x0400010E:
            timers.write_hi(halfword, 7);
            return;
        case 0x04000180:
            IPCSYNC_NDS9.write(halfword);
            IPCSYNC_NDS7.receive_input(halfword);

            if (halfword & (1 << 13) && IPCSYNC_NDS7.IRQ_enable)
                request_interrupt7(INTERRUPT::IPCSYNC);
            return;
        case 0x04000184:
            fifo9.write_CNT(halfword);
            if (fifo9.request_empty_IRQ)
            {
                request_interrupt9(INTERRUPT::IPC_FIFO_EMPTY);
                fifo9.request_empty_IRQ = false;
            }
            if (fifo9.request_nempty_IRQ)
            {
                request_interrupt9(INTERRUPT::IPC_FIFO_NEMPTY);
                fifo9.request_nempty_IRQ = false;
            }
            return;
        case 0x040001A0:
            cart.set_AUXSPICNT(halfword);
            return;
        case 0x04000204:
            EXMEMCNT = halfword;
            return;
        case 0x04000208:
            int9_reg.IME = halfword & 0x1;
            return;
        case 0x04000210:
            int9_reg.IE &= ~0xFFFF;
            int9_reg.IE |= halfword;
            return;
        case 0x04000212:
            int9_reg.IE &= ~0xFFFF0000;
            int9_reg.IE |= halfword << 16;
            return;
        case 0x04000248:
            gpu.set_VRAMCNT_H(halfword & 0xFF);
            gpu.set_VRAMCNT_I(halfword >> 8);
            return;
        case 0x04000280:
            DIVCNT = halfword;
            start_division();
            return;
        case 0x040002B0:
            SQRTCNT = halfword;
            start_sqrt();
            return;
        case 0x04000300:
            POSTFLG9 = halfword & 0x1;
            return;
        case 0x04000304:
            gpu.set_POWCNT1(halfword);
            return;
        case 0x04000340:
            //TODO: ALPHA_TEST_REF
            return;
        case 0x04000354:
            gpu.set_CLEAR_DEPTH(halfword);
            return;
        case 0x04000356:
            //TODO: CLRIMAGE_OFFSET
            return;
        case 0x0400035C:
            gpu.set_FOG_OFFSET(halfword);
            return;
        case 0x04001000:
            gpu.set_DISPCNT_B_lo(halfword);
            return;
        case 0x04001008:
            gpu.set_BGCNT_B(halfword, 0);
            return;
        case 0x0400100A:
            gpu.set_BGCNT_B(halfword, 1);
            return;
        case 0x0400100C:
            gpu.set_BGCNT_B(halfword, 2);
            return;
        case 0x0400100E:
            gpu.set_BGCNT_B(halfword, 3);
            return;
        case 0x04001010:
            gpu.set_BGHOFS_B(halfword, 0);
            return;
        case 0x04001012:
            gpu.set_BGVOFS_B(halfword, 0);
            return;
        case 0x04001014:
            gpu.set_BGHOFS_B(halfword, 1);
            return;
        case 0x04001016:
            gpu.set_BGVOFS_B(halfword, 1);
            return;
        case 0x04001018:
            gpu.set_BGHOFS_B(halfword, 2);
            return;
        case 0x0400101A:
            gpu.set_BGVOFS_B(halfword, 2);
            return;
        case 0x0400101C:
            gpu.set_BGHOFS_B(halfword, 3);
            return;
        case 0x0400101E:
            gpu.set_BGVOFS_B(halfword, 3);
            return;
        case 0x04001020:
            gpu.set_BG2P_B(halfword, 0);
            return;
        case 0x04001022:
            gpu.set_BG2P_B(halfword, 1);
            return;
        case 0x04001024:
            gpu.set_BG2P_B(halfword, 2);
            return;
        case 0x04001026:
            gpu.set_BG2P_B(halfword, 3);
            return;
        case 0x04001030:
            gpu.set_BG3P_B(halfword, 0);
            return;
        case 0x04001032:
            gpu.set_BG3P_B(halfword, 1);
            return;
        case 0x04001034:
            gpu.set_BG3P_B(halfword, 2);
            return;
        case 0x04001036:
            gpu.set_BG3P_B(halfword, 3);
            return;
        case 0x04001040:
            gpu.set_WIN0H_B(halfword);
            return;
        case 0x04001042:
            gpu.set_WIN1H_B(halfword);
            return;
        case 0x04001044:
            gpu.set_WIN0V_B(halfword);
            return;
        case 0x04001046:
            gpu.set_WIN1V_B(halfword);
            return;
        case 0x04001048:
            gpu.set_WININ_B(halfword);
            return;
        case 0x0400104A:
            gpu.set_WINOUT_B(halfword);
            return;
        case 0x0400104C:
            gpu.set_MOSAIC_B(halfword);
            return;
        case 0x04001050:
            gpu.set_BLDCNT_B(halfword);
            return;
        case 0x04001052:
            gpu.set_BLDALPHA_B(halfword);
            return;
        case 0x04001054:
            gpu.set_BLDY_B(halfword & 0x1F);
            return;
        case 0x0400106C:
            gpu.set_MASTER_BRIGHT_B(halfword);
            return;
    }
    if (address >= 0x04000330 && address < 0x04000340)
    {
        gpu.set_EDGE_COLOR(address, halfword);
        return;
    }
    if (address >= 0x04000380 && address < 0x040003C0)
    {
        gpu.set_TOON_TABLE((address & 0x3F) >> 1, halfword);
        return;
    }
    if (address >= GBA_ROM_START)
    {
        return;
    }
    printf("\n(9) Unrecognized halfword write of $%04X to $%08X", halfword, address);
    //exit(1);
}

void Emulator::arm9_write_byte(uint32_t address, uint8_t byte)
{
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        main_RAM[address & MAIN_RAM_MASK] = byte;
        return;
    }
    if (address >= IO_REGS_START && address < GBA_ROM_START)
    {
        if (Config::test)
            printf("\nWrite8: $%08X $%02X", address, byte);
    }
    if (address >= PALETTE_START && address < VRAM_BGA_START)
        return;
    switch (address)
    {
        case 0x0400004C:
            gpu.set_MOSAIC_A(byte);
            return;
        case 0x040001A1:
            cart.set_hi_AUXSPICNT(byte);
            return;
        case 0x040001A2:
            printf("\nAUXSPIDATA: $%02X", byte);
            cart.set_AUXSPIDATA(byte);
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
        case 0x04000208:
            int9_reg.IME = byte & 0x1;
            return;
        case 0x04000240:
            gpu.set_VRAMCNT_A(byte);
            return;
        case 0x04000241:
            gpu.set_VRAMCNT_B(byte);
            return;
        case 0x04000242:
            gpu.set_VRAMCNT_C(byte);
            return;
        case 0x04000243:
            gpu.set_VRAMCNT_D(byte);
            return;
        case 0x04000244:
            gpu.set_VRAMCNT_E(byte);
            return;
        case 0x04000245:
            gpu.set_VRAMCNT_F(byte);
            return;
        case 0x04000246:
            gpu.set_VRAMCNT_G(byte);
            return;
        case 0x04000247:
            WRAMCNT = byte & 0x3;
            return;
        case 0x04000248:
            gpu.set_VRAMCNT_H(byte);
            return;
        case 0x04000249:
            gpu.set_VRAMCNT_I(byte);
            return;
        case 0x0400104C:
            gpu.set_MOSAIC_B(byte);
            return;
        case 0x04001054:
            gpu.set_BLDY_B(byte & 0x1F);
            return;
    }
    if (address >= PALETTE_START && address < GBA_ROM_START)
    {
        printf("\nWarning: 8-bit write to VRAM $%08X", address);
        return;
    }
    printf("\n(9) Unrecognized byte write of $%02X to $%08X", byte, address);
    //exit(1);
}
