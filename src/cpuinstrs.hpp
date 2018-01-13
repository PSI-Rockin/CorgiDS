/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef cpuinstrs_hpp
#define cpuinstrs_hpp
#include "cpu.hpp"
#include "instrtable.h"

namespace Interpreter
{
    typedef void (*interpreter_func)(ARM_CPU& cpu, uint32_t instruction);
    extern const interpreter_func arm_table[4096];

    void arm_interpret(ARM_CPU& cpu);
    void thumb_interpret(ARM_CPU& cpu);
    ARM_INSTR arm_decode(uint32_t instruction);
    THUMB_INSTR thumb_decode(uint32_t instruction);
    
    uint32_t load_store_shift_reg(ARM_CPU& cpu, uint32_t instruction);
    
    void undefined(ARM_CPU& cpu, uint32_t instruction);
    void data_processing(ARM_CPU& cpu, uint32_t instruction);
    void count_leading_zeros(ARM_CPU& cpu, uint32_t instruction);
    void saturated_op(ARM_CPU& cpu, uint32_t instruction);
    void multiply(ARM_CPU& cpu, uint32_t instruction);
    void multiply_long(ARM_CPU& cpu, uint32_t instruction);
    void signed_halfword_multiply(ARM_CPU& cpu, uint32_t instruction);
    void swap(ARM_CPU& cpu, uint32_t instruction);
    void store_word(ARM_CPU& cpu, uint32_t instruction);
    void load_word(ARM_CPU& cpu, uint32_t instruction);
    void store_byte(ARM_CPU& cpu, uint32_t instruction);
    void load_byte(ARM_CPU& cpu, uint32_t instruction);
    void store_halfword(ARM_CPU& cpu, uint32_t instruction);
    void load_halfword(ARM_CPU& cpu, uint32_t instruction);
    void load_signed_byte(ARM_CPU& cpu, uint32_t instruction);
    void load_signed_halfword(ARM_CPU& cpu, uint32_t instruction);
    void store_doubleword(ARM_CPU& cpu, uint32_t instruction);
    void load_doubleword(ARM_CPU& cpu, uint32_t instruction);
    void store_block(ARM_CPU& cpu, uint32_t instruction);
    void load_block(ARM_CPU& cpu, uint32_t instruction);
    void branch(ARM_CPU& cpu, uint32_t instruction);
    void branch_link(ARM_CPU& cpu, uint32_t instruction);
    void branch_exchange(ARM_CPU& cpu, uint32_t instruction);
    void coprocessor_reg_transfer(ARM_CPU& cpu, uint32_t instruction);
    void blx_reg(ARM_CPU& cpu, uint32_t instruction);
    void blx(ARM_CPU& cpu, uint32_t instruction);
    void swi(ARM_CPU& cpu, uint32_t instruction);
    
    void thumb_mov_shift(ARM_CPU& cpu);
    void thumb_add_reg(ARM_CPU& cpu);
    void thumb_sub_reg(ARM_CPU& cpu);
    void thumb_mov(ARM_CPU& cpu);
    void thumb_cmp(ARM_CPU& cpu);
    void thumb_add(ARM_CPU& cpu);
    void thumb_sub(ARM_CPU& cpu);
    void thumb_alu_op(ARM_CPU& cpu);
    void thumb_hi_reg_op(ARM_CPU& cpu);
    void thumb_pc_rel_load(ARM_CPU& cpu);
    void thumb_store_reg_offset(ARM_CPU& cpu);
    void thumb_load_reg_offset(ARM_CPU& cpu);
    void thumb_load_halfword(ARM_CPU& cpu);
    void thumb_store_halfword(ARM_CPU& cpu);
    void thumb_store_imm_offset(ARM_CPU& cpu);
    void thumb_load_imm_offset(ARM_CPU& cpu);
    void thumb_load_store_sign_halfword(ARM_CPU& cpu);
    void thumb_sp_rel_store(ARM_CPU& cpu);
    void thumb_sp_rel_load(ARM_CPU& cpu);
    void thumb_offset_sp(ARM_CPU& cpu);
    void thumb_load_address(ARM_CPU& cpu);
    void thumb_push(ARM_CPU& cpu);
    void thumb_pop(ARM_CPU& cpu);
    void thumb_store_multiple(ARM_CPU& cpu);
    void thumb_load_multiple(ARM_CPU& cpu);
    void thumb_branch(ARM_CPU& cpu);
    void thumb_cond_branch(ARM_CPU& cpu);
    void thumb_long_branch_prep(ARM_CPU& cpu);
    void thumb_long_branch(ARM_CPU& cpu);
    void thumb_long_blx(ARM_CPU& cpu);
};

#endif /* cpuinstrs_hpp */
