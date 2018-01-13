/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include "debugger.hpp"
#include "disassembler.hpp"
#include "emulator.hpp"

using namespace std;

Debugger::Debugger(Emulator* e) : e(e)
{
    memset(exec_region, 0, 1024 * 1024 * 4);
}

void Debugger::mark_as_data(uint32_t address)
{

}

void Debugger::mark_as_arm(uint32_t address)
{
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        address &= MAIN_RAM_MASK;
        *(uint32_t*)&exec_region[address] = 0x01010101;
    }
}

void Debugger::mark_as_thumb(uint32_t address)
{
    if (address >= MAIN_RAM_START && address < SHARED_WRAM_START)
    {
        address &= MAIN_RAM_MASK;
        *(uint16_t*)&exec_region[address] = 0x0202;
    }
}

void Debugger::dump_disassembly()
{
    int step = 2;
    int last_part = 0;
    uint32_t instruction;
    ARM_CPU* burp = e->get_arm9();
    ofstream file("disassembly.txt");
    if (!file.is_open())
    {
        printf("\nFile not open!");
        return;
    }
    for (int i = MAIN_RAM_START; i < MAIN_RAM_START + (1024 * 1024 * 4); i += step)
    {
        int new_part = exec_region[i & MAIN_RAM_MASK];
        switch (new_part)
        {
            case 0:
                //if (last_part != new_part)
                    //file << "..." << endl;
                step = 2;
                break;
            case 1:
                //file << "0x" << setfill('0') << setw(8) << hex << i << " ";
                instruction = e->arm9_read_word(i);
                file << /*"0x" << */setfill('0') << setw(8) << hex << instruction << "    ";
                //file << Disassembler::disasm_arm(*burp, instruction, i) << endl;
                step = 4;
                break;
            case 2:
                //if (last_part != new_part)
                    //file << endl << endl << "$" << std::hex << i << " (THUMB)";
                step = 2;
                break;
        }
        last_part = new_part;
    }
    file.close();
}
