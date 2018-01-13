/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef BIOS_HPP
#define BIOS_HPP
#include "cpu.hpp"

class Emulator;

class BIOS
{
    private:
        uint8_t get_opcode(ARM_CPU& cpu);

        void div(ARM_CPU& cpu);
        void cpu_set(ARM_CPU& cpu);
        void get_CRC16(ARM_CPU& cpu);
    public:
        BIOS();

        int SWI7(ARM_CPU& arm7);
        int SWI9(ARM_CPU& arm9);
};

#endif // BIOS_HPP
