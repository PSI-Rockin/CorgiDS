#include "../emulator.hpp"

uint32_t Emulator::gba_read_word(uint32_t address)
{
    if (address < 0x4000)
        return *(uint32_t*)&gba_bios[address];
    if (address >= 0x02000000 && address < 0x02040000)
        return gpu.read_ARM7<uint32_t>(address);
    if (address >= 0x03000000 && address < 0x03008000)
        return *(uint32_t*)&shared_WRAM[address & 0x7FFF];
    if (address >= 0x08000000 && address < 0x0A000000)
        return slot2.read<uint32_t>(address);
    printf("\nUnrecognized read32 $%08X", address);
    return 0;
    //throw "[GBA_MEM] Unrecognized read32";
}

uint16_t Emulator::gba_read_halfword(uint32_t address)
{
    if (address < 0x4000)
        return *(uint16_t*)&gba_bios[address];
    if (address >= 0x02000000 && address < 0x02040000)
        return gpu.read_ARM7<uint16_t>(address);
    if (address >= 0x03000000 && address < 0x03008000)
        return *(uint16_t*)&shared_WRAM[address & 0x7FFF];
    if (address >= 0x08000000 && address < 0x0A000000)
        return slot2.read<uint16_t>(address);
    printf("\nUnrecognized read16 $%08X", address);
    return 0;
    //throw "[GBA_MEM] Unrecognized read16";
}

uint8_t Emulator::gba_read_byte(uint32_t address)
{
    if (address < 0x4000)
        return gba_bios[address];
    if (address >= 0x02000000 && address < 0x02040000)
        return gpu.read_ARM7<uint8_t>(address);
    if (address >= 0x03000000 && address < 0x03008000)
        return shared_WRAM[address & 0x7FFF];
    if (address >= 0x08000000 && address < 0x0A000000)
        return slot2.read<uint8_t>(address);
    switch (address)
    {
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
    if (address >= 0x03000000 && address < 0x03008000)
    {
        *(uint32_t*)&shared_WRAM[address & 0x7FFF] = word;
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
    if (address >= 0x03000000 && address < 0x03008000)
    {
        *(uint16_t*)&shared_WRAM[address & 0x7FFF] = halfword;
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
    if (address >= 0x03000000 && address < 0x03008000)
    {
        shared_WRAM[address & 0x7FFF] = byte;
        return;
    }
    switch (address)
    {
        case 0x04000208:
            int7_reg.IME = byte & 0x1;
            return;
        case 0x04000300:
            POSTFLG7 = byte & 0x1;
            return;
    }
    printf("\nUnrecognized write8 $%08X $%02X", address, byte);
    //throw "[GBA_MEM] Unrecognized write8";
}
