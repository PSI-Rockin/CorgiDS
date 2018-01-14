/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include "cp15.hpp"
#include "cpu.hpp"
#include "emulator.hpp"

CP15::CP15(Emulator* e) : e(e)
{
    //Doesn't reflect true initial values, but does give us a basis when executing the BIOS
    control.set_values(0);
    dtcm_base = 0x0B000000;
    dtcm_size = 0x00010000;
    itcm_size = 0x02000000;
    arm9 = nullptr;
}

void ControlReg::set_values(uint32_t reg)
{
    mmu_pu_enable = (reg & 0x1) != 0;
    data_unified_cache_on = (reg & (1 << 2)) != 0;
    is_big_endian = (reg & (1 << 7)) != 0;
    instruction_cache_on = (reg & (1 << 12)) != 0;
    high_exception_vector = (reg & (1 << 13)) != 0;
    predictable_cache_replacement = (reg & (1 << 14)) != 0;
    pre_ARMv5_mode = (reg & (1 << 15)) != 0;
    dtcm_enable = (reg & (1 << 16)) != 0;
    dtcm_write_only = (reg & (1 << 17)) != 0;
    itcm_enable = (reg & (1 << 18)) != 0;
    itcm_write_only = (reg & (1 << 19)) != 0;
}

uint32_t ControlReg::get_values()
{
    uint32_t reg = 0;
    reg |= mmu_pu_enable;
    reg |= data_unified_cache_on << 2;
    reg |= 0xF << 3;
    reg |= is_big_endian << 7;
    reg |= instruction_cache_on << 12;
    reg |= high_exception_vector << 13;
    reg |= predictable_cache_replacement << 14;
    reg |= pre_ARMv5_mode << 15;
    reg |= dtcm_enable << 16;
    reg |= dtcm_write_only << 17;
    reg |= itcm_enable << 18;
    reg |= itcm_write_only << 19;
    return reg;
}

void CP15::power_on()
{
    itcm_size = 0x02000000;
    dtcm_size = 16 * 1024;
    dtcm_base = 0x0B000000;
    control.itcm_enable = true;
    control.dtcm_enable = true;
}

void CP15::link_with_cpu(ARM_CPU *arm9)
{
    this->arm9 = arm9;
}

void CP15::mcr(int operation, int destination_reg, int ARM_reg_contents, int info, int operand_reg)
{
    uint32_t cp15_op = (destination_reg << 8) | (operand_reg << 4) | info;
    if (cp15_op >= 0x600 && cp15_op < 0x700) //Protection unit shit
        return;
    switch (cp15_op)
    {
        case 0x100:
            control.set_values(ARM_reg_contents);
            break;
        case 0x200:
            //printf("\nCachability for data protection: $%02X", ARM_reg_contents & 0xFF);
            break;
        case 0x201:
            //printf("\nCachability for instruction protection: $%02X", ARM_reg_contents & 0xFF);
            break;
        case 0x300:
            //printf("\nWrite bufferability for data protection: $%02X", ARM_reg_contents & 0xFF);
            break;
        case 0x502:
            break;
        case 0x503:
            break;
        case 0x704:
            arm9->halt();
            break;
        case 0x750:
            //printf("\nInvalidate instruction cache");
            break;
        case 0x751:
            //printf("\nInvalidate instruction cache line");
            break;
        case 0x760:
            //printf("\nInvalidate data cache");
            break;
        case 0x761:
            //printf("\nInvalidate data cache line");
            break;
        case 0x7A1:
            //printf("\nClean data cache line");
            break;
        case 0x7A2:
            //printf("\nClean data cache line");
            break;
        case 0x7A4:
            //printf("\nDrain write buffer");
            break;
        case 0x7E1:
            //printf("\nClean and invalidate data cache line");
            break;
        case 0x7E2:
            //printf("\nClean and invalidate data cache line");
            break;
        case 0x910:
            dtcm_data = ARM_reg_contents;
            dtcm_base = dtcm_data >> 12;
            dtcm_base <<= 12;
            dtcm_size = (dtcm_data >> 1) & 0x1F;
            dtcm_size = 512 << dtcm_size;
            printf("\nDTCM base: $%08X", get_dtcm_base());
            printf("\nDTCM size: $%08X", get_dtcm_size());
            break;
        case 0x911:
            itcm_data = ARM_reg_contents;
            itcm_size = (itcm_data >> 1) & 0x1F;
            itcm_size = 512 << itcm_size;
            printf("\nITCM size: $%08X", get_itcm_size());
            break;
        default:
            printf("\nUnrecognized MCR op $%03X", cp15_op);
            throw "[CP15] Unrecognized MCR op";
    }
}

uint32_t CP15::mrc(int operation, int source_reg, int info, int operand_reg)
{
    uint32_t cp15_op = (source_reg << 8) | (operand_reg << 4) | info;
    switch (cp15_op)
    {
        case 0x000:
            return 0x41059461; //This magic number comes from GBAtek, too lazy to figure out what it means
        case 0x001:
            return 0x0F0D2112;
        case 0x100:
            return control.get_values();
        case 0x502:
            return 0; //I dunno lol
        case 0x620:
            return 0;
        case 0x910:
            return dtcm_data;
        default:
            printf("\nUnrecognized MRC op $%03X", cp15_op);
            throw "[CP15] Unrecognized MRC op";
    }
}

uint32_t CP15::read_word(uint32_t address)
{
    if (address < itcm_size)
    {
        return *(uint32_t*)&ITCM[address & ITCM_MASK];
    }
    else if (address >= dtcm_base && address < (dtcm_base + dtcm_size) && !control.dtcm_write_only)
    {
        if (address == 0x2076EC0)
            printf("DERP");
        return *(uint32_t*)&DTCM[address & DTCM_MASK];
    }
    return e->arm9_read_word(address);
}

uint16_t CP15::read_halfword(uint32_t address)
{
    if (address < itcm_size)
    {
        return *(uint16_t*)&ITCM[address & ITCM_MASK];
    }
    else if (address >= dtcm_base && address < (dtcm_base + dtcm_size))
    {
        return *(uint16_t*)&DTCM[address & DTCM_MASK];
    }
    return e->arm9_read_halfword(address);
}

uint8_t CP15::read_byte(uint32_t address)
{
    if (address < itcm_size)
    {
        return ITCM[address & ITCM_MASK];
    }
    else if (address >= dtcm_base && address < (dtcm_base + dtcm_size))
    {
        return DTCM[address & DTCM_MASK];
    }
    return e->arm9_read_byte(address);
}

void CP15::write_word(uint32_t address, uint32_t word)
{
    if (address < itcm_size)
    {
        *(uint32_t*)&ITCM[address & ITCM_MASK] = word;
    }
    else if (address >= dtcm_base && address < (dtcm_base + dtcm_size))
    {
        *(uint32_t*)&DTCM[address & DTCM_MASK] = word;
    }
    else
    {
        e->arm9_write_word(address, word);
    }
}

void CP15::write_halfword(uint32_t address, uint16_t halfword)
{
    if (address < itcm_size)
    {
        *(uint16_t*)&ITCM[address & ITCM_MASK] = halfword;
    }
    else if (address >= dtcm_base && address < (dtcm_base + dtcm_size))
    {
        *(uint16_t*)&DTCM[address & DTCM_MASK] = halfword;
    }
    else
    {
        e->arm9_write_halfword(address, halfword);
    }
}

void CP15::write_byte(uint32_t address, uint32_t byte)
{
    if (address < itcm_size)
    {
        ITCM[address & ITCM_MASK] = byte;
    }
    else if (address >= dtcm_base && address < (dtcm_base + dtcm_size))
    {
        DTCM[address & DTCM_MASK] = byte;
    }
    else
    {
        e->arm9_write_byte(address, byte);
    }
}
