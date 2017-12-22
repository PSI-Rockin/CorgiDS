/*
    CorgiDS Copyright PSISP 2017
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H
#include <string>
#include "cpuinstrs.hpp"

namespace Disassembler
{
    std::string disasm_arm(ARM_CPU& cpu, uint32_t instruction, uint32_t address);
    std::string disasm_data_processing(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_msr(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_mrs(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_load_store(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_branch(ARM_CPU& cpu, uint32_t instruction, uint32_t address);
    std::string disasm_bx(uint32_t instruction);

    std::string disasm_thumb(ARM_CPU& cpu, uint16_t instruction, uint32_t address);
};

#endif // DISASSEMBLER_H
