/*
    CorgiDS Copyright PSISP 2017-2018
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
    std::string disasm_count_leading_zeros(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_multiply(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_multiply_long(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_signed_halfword_multiply(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_swap(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_load_store(ARM_CPU& cpu, uint32_t instruction, uint32_t address);
    std::string disasm_load_store_halfword(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_load_signed(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_load_store_block(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_branch(ARM_CPU& cpu, uint32_t instruction, uint32_t address);
    std::string disasm_bx(ARM_CPU& cpu, uint32_t instruction);
    std::string disasm_coprocessor_reg_transfer(ARM_CPU& cpu, uint32_t instruction);

    std::string disasm_thumb(ARM_CPU& cpu, uint16_t instruction, uint32_t address);
    std::string disasm_thumb_mov_shift(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_op_reg(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_op_imm(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_alu_op(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_hi_reg_op(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_pc_rel_load(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_load_store_reg(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_load_store_signed(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_load_store_halfword(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_load_store_imm(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_sp_rel_load_store(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_offset_sp(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_load_address(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_push_pop(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_load_store_multiple(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_branch(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_cond_branch(ARM_CPU& cpu, uint16_t instruction);
    std::string disasm_thumb_long_branch(ARM_CPU& cpu, uint32_t instruction);
};

#endif // DISASSEMBLER_H
