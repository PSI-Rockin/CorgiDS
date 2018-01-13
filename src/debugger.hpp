/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP
#include <cstdint>
#include "memconsts.h"

class Emulator;

class Debugger
{
    private:
        Emulator* e;

        uint8_t exec_region[1024 * 1024 * 4];
    public:
        Debugger(Emulator* e);

        void mark_as_data(uint32_t address);
        void mark_as_arm(uint32_t address);
        void mark_as_thumb(uint32_t address);
        void dump_disassembly();
};

#endif // DEBUGGER_HPP
