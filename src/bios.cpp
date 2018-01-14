/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include <iomanip>
#include <sstream>
#include "bios.hpp"

using namespace std;

BIOS::BIOS()
{

}

uint8_t BIOS::get_opcode(ARM_CPU &cpu)
{
    //The SWI handler retrieves the opcode based upon the value of LR
    uint32_t LR = cpu.get_PC();
    if (cpu.get_CPSR()->thumb_on)
        LR -= 2;
    else
        LR -= 4;

    return cpu.read_halfword(LR - 2) & 0xFF;
}

void BIOS::div(ARM_CPU &cpu)
{
    int32_t dividend = (int32_t)cpu.get_register(0);
    int32_t divisor = (int32_t)cpu.get_register(1);
    int32_t quotient = dividend / divisor;

    cpu.set_register(0, quotient);
    cpu.set_register(1, (dividend % divisor));
    cpu.set_register(3, abs(quotient));
}

void BIOS::cpu_set(ARM_CPU &cpu)
{
    uint32_t source = cpu.get_register(0);
    uint32_t dest = cpu.get_register(1);
    uint32_t flags = cpu.get_register(2);

    if (cpu.get_id())
    {
        //Reject reads and writes to BIOS area
        if (source < 0x4000 || dest < 0x4000)
            return;
    }

    uint32_t len = flags & 0x1FFFFF;
    bool memfill = flags & (1 << 24);
    bool size_word = flags & (1 << 26);

    while (len != 0)
    {
        if (size_word)
        {
            uint32_t value = cpu.read_word(source);
            cpu.write_word(dest, value);
        }
        else
        {
            uint16_t value = cpu.read_halfword(source);
            cpu.write_halfword(dest, value);
        }
        if (!memfill)
            source += 2 << size_word;
        dest += 2 << size_word;
        len--;
    }
}

void BIOS::get_CRC16(ARM_CPU &cpu)
{
    uint16_t CRCs[] = {0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001, 0xA001};
    uint16_t CRC = cpu.get_register(0) & 0xFFFF;
    uint32_t addr = cpu.get_register(1);
    uint32_t len = cpu.get_register(2);
    uint32_t end = addr + len;
    for (; addr < end; addr += 2)
    {
        CRC ^= cpu.read_halfword(addr);
        for (int i = 0; i < 8; i++)
        {
            bool carry = CRC & 0x1;
            CRC >>= 1;
            if (carry)
                CRC ^= (CRCs[i] << (7 - i));
        }
    }
    cpu.set_register(0, CRC);
}

int BIOS::SWI7(ARM_CPU &arm7)
{
    int opcode = get_opcode(arm7);
    switch (opcode)
    {
        case 0x3:
            arm7.add_internal_cycles(arm7.get_register(0) * 4);
            break;
        case 0x6:
            arm7.halt();
            break;
        case 0xB:
            cpu_set(arm7);
            break;
        case 0xE:
            get_CRC16(arm7);
            break;
        default:
            printf("\nUnrecognized ARM9 SWI $%02X", opcode);
            throw "Unrecognized ARM7 SWI";
    }
    return 1;
}

int BIOS::SWI9(ARM_CPU &arm9)
{
    int opcode = get_opcode(arm9);
    switch (opcode)
    {
        case 0x3:
            arm9.add_internal_cycles(arm9.get_register(0) * 2);
            break;
        case 0x6:
            arm9.halt();
            break;
        case 0xB:
            cpu_set(arm9);
            break;
        case 0xE:
            get_CRC16(arm9);
            break;
        default:
            printf("\nUnrecognized ARM9 SWI $%02X", opcode);
            throw "Unrecognized ARM9 SWI";
    }
    return 1;
}
