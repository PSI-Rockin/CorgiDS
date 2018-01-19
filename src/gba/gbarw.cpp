#include "../emulator.hpp"

uint32_t Emulator::gba_read_word(uint32_t address)
{
    if (address < 0x4000)
        return *(uint32_t*)&gba_bios[address];
    if (address >= 0x02000000 && address < 0x02040000)
        return gpu.read_ARM7<uint32_t>(address + 0x04000000);
    if (address >= 0x03000000 && address < 0x04000000)
        return *(uint32_t*)&shared_WRAM[address & 0x7FFF];
    if (address >= 0x06000000 && address < 0x07000000)
        return gpu.read_gba<uint32_t>(address);
    if (address >= OAM_START && address < GBA_ROM_START)
        return gpu.read_OAM<uint32_t>(address & 0x3FF);
    if (address >= 0x08000000 && address < 0x0A000000)
        return slot2.read<uint32_t>(address);
    switch (address)
    {
        case 0x040000DC:
            return gba_dma.read_len_CNT(3);
        case 0x04000200:
            return int7_reg.IE | (int7_reg.IF << 16);
    }
    printf("\nUnrecognized read32 $%08X", address);
    return 0;
    //throw "[GBA_MEM] Unrecognized read32";
}

uint16_t Emulator::gba_read_halfword(uint32_t address)
{
    if (address < 0x4000)
        return *(uint16_t*)&gba_bios[address];
    if (address >= 0x02000000 && address < 0x02040000)
        return gpu.read_ARM7<uint16_t>(address + 0x04000000);
    if (address >= 0x03000000 && address < 0x04000000)
        return *(uint16_t*)&shared_WRAM[address & 0x7FFF];
    if (address >= 0x06000000 && address < 0x07000000)
        return gpu.read_gba<uint16_t>(address);
    if (address >= OAM_START && address < GBA_ROM_START)
        return gpu.read_OAM<uint16_t>(address & 0x3FF);
    if (address >= 0x08000000 && address < 0x0A000000)
        return slot2.read<uint16_t>(address);
    switch (address)
    {
        case 0x04000004:
            return gpu.get_DISPSTAT7();
        case 0x04000006:
            return gpu.get_VCOUNT();
        case 0x04000088:
            return spu.get_SOUNDBIAS() << 1;
        case 0x040000BA:
            return gba_dma.read_CNT(0);
        case 0x04000130:
            return KEYINPUT.get();
        case 0x04000200:
            return int7_reg.IE;
        case 0x04000202:
            return int7_reg.IF;
    }
    printf("\nUnrecognized read16 $%08X", address);
    return 0;
    //throw "[GBA_MEM] Unrecognized read16";
}

uint8_t Emulator::gba_read_byte(uint32_t address)
{
    if (address < 0x4000)
        return gba_bios[address];
    if (address >= 0x02000000 && address < 0x02040000)
        return gpu.read_ARM7<uint8_t>(address + 0x04000000);
    if (address >= 0x03000000 && address < 0x04000000)
        return shared_WRAM[address & 0x7FFF];
    if (address >= 0x08000000 && address < 0x0A000000)
        return slot2.read<uint8_t>(address);
    switch (address)
    {
        case 0x04000006:
            return gpu.get_VCOUNT();
        case 0x04000300:
            return POSTFLG7;
    }
    printf("\nUnrecognized read8 $%08X", address);
    return 0;
    //throw "[GBA_MEM] Unrecognized read8";
}

void Emulator::gba_write_word(uint32_t address, uint32_t word)
{
    if (address >= 0x02000000 && address < 0x02040000)
    {
        gpu.write_ARM7<uint32_t>(address + 0x04000000, word);
        return;
    }
    if (address >= 0x03000000 && address < 0x04000000)
    {
        *(uint32_t*)&shared_WRAM[address & 0x7FFF] = word;
        return;
    }
    if (address >= 0x05000000 && address < 0x06000000)
    {
        gpu.write_palette_A(address, word & 0xFFFF);
        gpu.write_palette_A(address + 2, word >> 16);
        return;
    }
    if (address >= 0x06000000 && address < 0x07000000)
    {
        gpu.write_gba<uint32_t>(address, word);
        return;
    }
    if (address >= OAM_START && address < GBA_ROM_START)
    {
        gpu.write_OAM(address & 0x3FF, word & 0xFFFF);
        gpu.write_OAM((address + 2) & 0x3FF, word >> 16);
        return;
    }
    switch (address)
    {
        case 0x04000008:
            gpu.set_BGCNT_A(word & 0xFFFF, 0);
            gpu.set_BGCNT_A(word >> 16, 1);
            return;
        case 0x0400000C:
            gpu.set_BGCNT_A(word & 0xFFFF, 2);
            gpu.set_BGCNT_A(word >> 16, 3);
            return;
        case 0x040000D4:
            gba_dma.write_source(3, word);
            return;
        case 0x040000D8:
            gba_dma.write_dest(3, word);
            return;
        case 0x040000DC:
            gba_dma.write_len_CNT(3, word);
            return;
    }
    printf("\nUnrecognized write32 $%08X $%08X", address, word);
    //throw "[GBA_MEM] Unrecognized write32";
}

void Emulator::gba_write_halfword(uint32_t address, uint16_t halfword)
{
    if (address >= 0x02000000 && address < 0x02040000)
    {
        gpu.write_ARM7<uint16_t>(address + 0x04000000, halfword);
        return;
    }
    if (address >= 0x03000000 && address < 0x04000000)
    {
        *(uint16_t*)&shared_WRAM[address & 0x7FFF] = halfword;
        return;
    }
    if (address >= 0x05000000 && address < 0x06000000)
    {
        gpu.write_palette_A(address, halfword);
        return;
    }
    if (address >= 0x06000000 && address < 0x07000000)
    {
        gpu.write_gba<uint16_t>(address, halfword);
        return;
    }
    if (address >= OAM_START && address < GBA_ROM_START)
    {
        gpu.write_OAM(address & 0x3FF, halfword);
        return;
    }
    switch (address)
    {
        case 0x04000000:
            gpu.gba_set_DISPCNT(halfword);
            return;
        case 0x04000004:
            gpu.set_DISPSTAT7(halfword);
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
        case 0x04000088:
            spu.set_SOUNDBIAS((halfword >> 1) & 0x1FF);
            return;
        case 0x040000BA:
            gba_dma.write_CNT(0, halfword);
            return;
        case 0x04000200:
            int7_reg.IE = halfword;
            return;
        case 0x04000202:
            int7_reg.IF &= ~halfword;
            return;
        case 0x04000208:
            int7_reg.IME = halfword & 0x1;
            return;
    }
    printf("\nUnrecognized write16 $%08X $%04X", address, halfword);
    //throw "[GBA_MEM] Unrecognized write16";
}

void Emulator::gba_write_byte(uint32_t address, uint8_t byte)
{
    if (address >= 0x02000000 && address < 0x02040000)
    {
        gpu.write_ARM7<uint8_t>(address + 0x04000000, byte);
        return;
    }
    if (address >= 0x03000000 && address < 0x04000000)
    {
        shared_WRAM[address & 0x7FFF] = byte;
        return;
    }
    switch (address)
    {
        case 0x04000202:
            int7_reg.IF &= 0xFF00 | ~byte;
            return;
        case 0x04000203:
            int7_reg.IF &= 0xFF | ~(byte << 8);
            return;
        case 0x04000208:
            int7_reg.IME = byte & 0x1;
            return;
        case 0x04000300:
            POSTFLG7 = byte & 0x1;
            return;
        case 0x04000301:
            //TODO: sleep
            arm7.halt();
            return;
    }
    printf("\nUnrecognized write8 $%08X $%02X", address, byte);
    //throw "[GBA_MEM] Unrecognized write8";
}
