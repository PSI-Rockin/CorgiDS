/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include "config.hpp"
#include "cpuinstrs.hpp"
#include "disassembler.hpp"

void Interpreter::thumb_interpret(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr() & 0xFFFF;

    THUMB_INSTR opcode = thumb_decode(instruction);
    
    if (cpu.get_id() && Config::test)
    {
        if (opcode != THUMB_INSTR::LONG_BLX && opcode != THUMB_INSTR::LONG_BRANCH)
        {
            printf("\n");
            uint32_t PC = cpu.get_PC() - 4;
            if (!cpu.get_id())
                printf("(9T)");
            else
                printf("(7T)");
            if (opcode == THUMB_INSTR::LONG_BRANCH_PREP)
            {
                uint32_t long_instr = instruction | (cpu.read_halfword(PC + 2) << 16);
                printf("[$%08X] {$%08X} - ", PC, long_instr);
                printf("%s", Disassembler::disasm_thumb_long_branch(cpu, long_instr).c_str());
            }
            else
            {
                printf("[$%08X] {$%04X} - ", PC, instruction);
                printf("%s", Disassembler::disasm_thumb(cpu, instruction, PC).c_str());
            }
        }
    }
    
    switch (opcode)
    {
        case THUMB_INSTR::MOV_SHIFT:
            thumb_mov_shift(cpu);
            break;
        case THUMB_INSTR::ADD_REG:
            thumb_add_reg(cpu);
            break;
        case THUMB_INSTR::SUB_REG:
            thumb_sub_reg(cpu);
            break;
        case THUMB_INSTR::MOV_IMM:
            thumb_mov(cpu);
            break;
        case THUMB_INSTR::CMP_IMM:
            thumb_cmp(cpu);
            break;
        case THUMB_INSTR::ADD_IMM:
            thumb_add(cpu);
            break;
        case THUMB_INSTR::SUB_IMM:
            thumb_sub(cpu);
            break;
        case THUMB_INSTR::ALU_OP:
            thumb_alu_op(cpu);
            break;
        case THUMB_INSTR::HI_REG_OP:
            thumb_hi_reg_op(cpu);
            break;
        case THUMB_INSTR::PC_REL_LOAD:
            thumb_pc_rel_load(cpu);
            break;
        case THUMB_INSTR::STORE_IMM_OFFSET:
            thumb_store_imm_offset(cpu);
            break;
        case THUMB_INSTR::LOAD_IMM_OFFSET:
            thumb_load_imm_offset(cpu);
            break;
        case THUMB_INSTR::STORE_REG_OFFSET:
            thumb_store_reg_offset(cpu);
            break;
        case THUMB_INSTR::LOAD_REG_OFFSET:
            thumb_load_reg_offset(cpu);
            break;
        case THUMB_INSTR::STORE_HALFWORD:
            thumb_store_halfword(cpu);
            break;
        case THUMB_INSTR::LOAD_HALFWORD:
            thumb_load_halfword(cpu);
            break;
        case THUMB_INSTR::LOAD_STORE_SIGN_HALFWORD:
            thumb_load_store_sign_halfword(cpu);
            break;
        case THUMB_INSTR::SP_REL_STORE:
            thumb_sp_rel_store(cpu);
            break;
        case THUMB_INSTR::SP_REL_LOAD:
            thumb_sp_rel_load(cpu);
            break;
        case THUMB_INSTR::OFFSET_SP:
            thumb_offset_sp(cpu);
            break;
        case THUMB_INSTR::LOAD_ADDRESS:
            thumb_load_address(cpu);
            break;
        case THUMB_INSTR::LOAD_MULTIPLE:
            thumb_load_multiple(cpu);
            break;
        case THUMB_INSTR::STORE_MULTIPLE:
            thumb_store_multiple(cpu);
            break;
        case THUMB_INSTR::PUSH:
            thumb_push(cpu);
            break;
        case THUMB_INSTR::POP:
            thumb_pop(cpu);
            break;
        case THUMB_INSTR::BRANCH:
            thumb_branch(cpu);
            break;
        case THUMB_INSTR::COND_BRANCH:
            thumb_cond_branch(cpu);
            break;
        case THUMB_INSTR::LONG_BRANCH_PREP:
            thumb_long_branch_prep(cpu);
            break;
        case THUMB_INSTR::LONG_BRANCH:
            thumb_long_branch(cpu);
            break;
        case THUMB_INSTR::LONG_BLX:
            thumb_long_blx(cpu);
            break;
        default:
            printf("\nUnrecognized Thumb opcode $%04X", cpu.get_current_instr());
            cpu.handle_UNDEFINED();
    }
}

#define printf(fmt, ...)(0)

THUMB_INSTR Interpreter::thumb_decode(uint32_t instruction)
{
    uint16_t instr13 = instruction >> 13;
    uint16_t instr12 = instruction >> 12;
    uint16_t instr11 = instruction >> 11;
    uint16_t instr10 = instruction >> 10;
    switch (instr11)
    {
        case 0x4:
            return THUMB_INSTR::MOV_IMM;
        case 0x5:
            return THUMB_INSTR::CMP_IMM;
        case 0x6:
            return THUMB_INSTR::ADD_IMM;
        case 0x7:
            return THUMB_INSTR::SUB_IMM;
        case 0x9:
            return THUMB_INSTR::PC_REL_LOAD;
        case 0x10:
            return THUMB_INSTR::STORE_HALFWORD;
        case 0x11:
            return THUMB_INSTR::LOAD_HALFWORD;
        case 0x12:
            return THUMB_INSTR::SP_REL_STORE;
        case 0x13:
            return THUMB_INSTR::SP_REL_LOAD;
        case 0x18:
            return THUMB_INSTR::STORE_MULTIPLE;
        case 0x19:
            return THUMB_INSTR::LOAD_MULTIPLE;
        case 0x1C:
            return THUMB_INSTR::BRANCH;
        case 0x1D:
            return THUMB_INSTR::LONG_BLX;
    }
    if (instr13 == 0)
    {
        if ((instr11 & 0x3) != 0x3)
            return THUMB_INSTR::MOV_SHIFT;
        else
        {
            if ((instruction & (1 << 9)) != 0)
                return THUMB_INSTR::SUB_REG;
            return THUMB_INSTR::ADD_REG;
        }
    }
    if (instr10 == 0x10)
        return THUMB_INSTR::ALU_OP;
    if (instr10 == 0x11)
        return THUMB_INSTR::HI_REG_OP;
    if (instr12 == 0x5)
    {
        if ((instruction & (1 << 9)) == 0)
        {
            if ((instruction & (1 << 11)) == 0)
                return THUMB_INSTR::STORE_REG_OFFSET;
            return THUMB_INSTR::LOAD_REG_OFFSET;
        }
        return THUMB_INSTR::LOAD_STORE_SIGN_HALFWORD;
    }
    if (instr13 == 0x3)
    {
        if ((instruction & (1 << 11)) == 0)
            return THUMB_INSTR::STORE_IMM_OFFSET;
        return THUMB_INSTR::LOAD_IMM_OFFSET;
    }
    if (instr12 == 0xA)
        return THUMB_INSTR::LOAD_ADDRESS;
    if (instr12 == 0xB)
    {
        if (((instruction >> 9) & 0x3) == 0x2)
        {
            if ((instruction & (1 << 11)) != 0)
                return THUMB_INSTR::POP;
            return THUMB_INSTR::PUSH;
        }
        return THUMB_INSTR::OFFSET_SP;
    }
    if (instr12 == 0xD)
        return THUMB_INSTR::COND_BRANCH;
    if (instr12 == 0xF)
    {
        if ((instruction & (1 << 11)) == 0)
            return THUMB_INSTR::LONG_BRANCH_PREP;
        return THUMB_INSTR::LONG_BRANCH;
    }
    return THUMB_INSTR::UNDEFINED;
}

void Interpreter::thumb_mov_shift(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    int opcode = (instruction >> 11) & 0x3;
    int shift = (instruction >> 6) & 0x1F;
    uint32_t source = (instruction >> 3) & 0x7;
    uint32_t destination = instruction & 0x7;
    uint32_t value = cpu.get_register(source);
    
    switch (opcode)
    {
        case 0:
            value = cpu.lsl(value, shift, true);
            if (cpu.get_id())
                printf("LSL {%d}, {%d}, #%d", destination, source, shift);
            break;
        case 1:
            if (!shift)
                shift = 32;
            value = cpu.lsr(value, shift, true);
            if (cpu.get_id())
                printf("LSR {%d}, {%d}, #%d", destination, source, shift);
            break;
        case 2:
            if (!shift)
                shift = 32;
            value = cpu.asr(value, shift, true);
            if (cpu.get_id())
                printf("ASR {%d}, {%d}, #%d", destination, source, shift);
            break;
        default:
            printf("Unrecognized opcode %d in thumb_mov_shift", opcode);
            throw "[THUMB_INSTR] Unrecognized thumb_mov_shift opcode";
    }
    
    cpu.add_internal_cycles(1); //Extra cycle due to register shift
    cpu.set_register(destination, value);
    //cpu.set_lo_register(destination, value);
}

void Interpreter::thumb_add_reg(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = instruction & 0x7;
    uint32_t source = (instruction >> 3) & 0x7;
    uint32_t operand = (instruction >> 6) & 0x7;
    bool is_imm = instruction & (1 << 10);
    
    if (!is_imm)
    {
        if (cpu.get_id())
            printf("ADD {%d}, {%d}, {%d}", destination, source, operand);
        operand = cpu.get_register(operand);
    }
    else
        if (cpu.get_id())
            printf("ADD {%d}, {%d}, $%08X", destination, source, operand);
    
    cpu.add(destination, cpu.get_register(source), operand, true);
}

void Interpreter::thumb_sub_reg(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = instruction & 0x7;
    uint32_t source = (instruction >> 3) & 0x7;
    uint32_t operand = (instruction >> 6) & 0x7;
    bool is_imm = instruction & (1 << 10);
    
    if (!is_imm)
    {
        if (cpu.get_id())
            printf("SUB {%d}, {%d}, {%d}", destination, source, operand);
        operand = cpu.get_register(operand);
    }
    else
        if (cpu.get_id())
            printf("SUB {%d}, {%d}, $%08X", destination, source, operand);
    
    cpu.sub(destination, cpu.get_register(source), operand, true);
}

void Interpreter::thumb_mov(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t offset = instruction & 0xFF;
    uint32_t reg = (instruction >> 8) & 0x7;
    
    if (cpu.get_id())
        printf("MOV {%d}, $%02X", reg, offset);
    
    cpu.mov(reg, offset, true);
}

void Interpreter::thumb_cmp(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t offset = instruction & 0xFF;
    uint32_t reg = (instruction >> 8) & 0x7;
    
    if (cpu.get_id())
        printf("CMP {%d}, $%02X", reg, offset);
    
    cpu.cmp(cpu.get_register(reg), offset);
}

void Interpreter::thumb_add(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t offset = instruction & 0xFF;
    uint32_t reg = (instruction >> 8) & 0x7;
    
    if (cpu.get_id())
        printf("ADD {%d}, $%02X", reg, offset);
    
    cpu.add(reg, cpu.get_register(reg), offset, true);
}

void Interpreter::thumb_sub(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t offset = instruction & 0xFF;
    uint32_t reg = (instruction >> 8) & 0x7;
    
    if (cpu.get_id())
        printf("SUB {%d}, $%02X", reg, offset);
    
    cpu.sub(reg, cpu.get_register(reg), offset, true);
}

void Interpreter::thumb_alu_op(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = instruction & 0x7;
    uint32_t source = (instruction >> 3) & 0x7;
    int opcode = (instruction >> 6) & 0xF;
    
    switch (opcode)
    {
        case 0x0:
            if (cpu.get_id())
                printf("AND {%d}, {%d}", destination, source);
            cpu.andd(destination, cpu.get_register(destination), cpu.get_register(source), true);
            break;
        case 0x1:
            if (cpu.get_id())
                printf("EOR {%d}, {%d}", destination, source);
            cpu.eor(destination, cpu.get_register(destination), cpu.get_register(source), true);
            break;
        case 0x2:
            if (cpu.get_id())
                printf("LSL {%d}, {%d}", destination, source);
        {
            uint32_t reg = cpu.get_register(destination);
            reg = cpu.lsl(reg, cpu.get_register(source), true);
            cpu.set_register(destination, reg);
        }
            break;
        case 0x3:
            if (cpu.get_id())
                printf("LSR {%d}, {%d}", destination, source);
        {
            uint32_t reg = cpu.get_register(destination);
            reg = cpu.lsr(reg, cpu.get_register(source), true);
            cpu.set_register(destination, reg);
        }
            break;
        case 0x4:
            if (cpu.get_id())
                printf("ASR {%d}, {%d}", destination, source);
        {
            uint32_t reg = cpu.get_register(destination);
            reg = cpu.asr(reg, cpu.get_register(source), true);
            cpu.set_register(destination, reg);
        }
            break;
        case 0x5:
            if (cpu.get_id())
                printf("ADC {%d}, {%d}", destination, source);
            cpu.adc(destination, cpu.get_register(destination), cpu.get_register(source), true);
            break;
        case 0x6:
            if (cpu.get_id())
                printf("SBC {%d}, {%d}", destination, source);
            cpu.sbc(destination, cpu.get_register(destination), cpu.get_register(source), true);
            break;
        case 0x7:
            if (cpu.get_id())
                printf("ROR {%d}, {%d}", destination, source);
        {
            uint32_t reg = cpu.get_register(destination);
            reg = cpu.rotr32(reg, cpu.get_register(source), true);
            cpu.set_register(destination, reg);
        }
            break;
        case 0x8:
            if (cpu.get_id())
                printf("TST {%d}, {%d}", destination, source);
            cpu.tst(cpu.get_register(destination), cpu.get_register(source));
            break;
        case 0x9:
            if (cpu.get_id())
                printf("NEG {%d}, {%d}", destination, source);
            //NEG is the same thing as RSBS Rd, Rs, #0
            cpu.sub(destination, 0, cpu.get_register(source), true);
            break;
        case 0xA:
            if (cpu.get_id())
                printf("CMP {%d}, {%d}", destination, source);
            cpu.cmp(cpu.get_register(destination), cpu.get_register(source));
            break;
        case 0xB:
            if (cpu.get_id())
                printf("CMN {%d}, {%d}", destination, source);
            cpu.cmn(cpu.get_register(destination), cpu.get_register(source));
            break;
        case 0xC:
            if (cpu.get_id())
                printf("ORR {%d}, {%d}", destination, source);
            cpu.orr(destination, cpu.get_register(destination), cpu.get_register(source), true);
            break;
        case 0xD:
            if (cpu.get_id())
                printf("MUL {%d}, {%d}", destination, source);
            cpu.mul(destination, cpu.get_register(destination), cpu.get_register(source), true);
            if (!cpu.get_id())
                cpu.add_internal_cycles(3);
            else
            {
                uint32_t multiplicand = cpu.get_register(destination);
                if (multiplicand & 0xFF000000)
                    cpu.add_internal_cycles(4);
                else if (multiplicand & 0x00FF0000)
                    cpu.add_internal_cycles(3);
                else if (multiplicand & 0x0000FF00)
                    cpu.add_internal_cycles(2);
                else
                    cpu.add_internal_cycles(1);
            }
            break;
        case 0xE:
            if (cpu.get_id())
                printf("BIC {%d}, {%d}", destination, source);
            cpu.bic(destination, cpu.get_register(destination), cpu.get_register(source), true);
            break;
        case 0xF:
            if (cpu.get_id())
                printf("MVN {%d}, {%d}", destination, source);
            cpu.mvn(destination, cpu.get_register(source), true);
            break;
        default:
            printf("\nInvalid thumb alu op %d", opcode);
    }
}

void Interpreter::thumb_hi_reg_op(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    int opcode = (instruction >> 8) & 0x3;
    bool high_source = (instruction & (1 << 6)) != 0;
    bool high_dest = (instruction & (1 << 7)) != 0;
    
    uint32_t source = (instruction >> 3) & 0x7;
    source |= high_source * 8;
    uint32_t destination = instruction & 0x7;
    destination |= high_dest * 8;
    
    switch (opcode)
    {
        case 0x0:
            if (cpu.get_id())
                printf("ADD {%d}, {%d}", destination, source);
            if (destination == REG_PC)
                cpu.jp(cpu.get_PC() + cpu.get_register(source), false);
            else
                cpu.add(destination, cpu.get_register(destination), cpu.get_register(source), false);
            break;
        case 0x1:
            if (cpu.get_id())
                printf("CMP {%d}, {%d}", destination, source);
            cpu.cmp(cpu.get_register(destination), cpu.get_register(source));
            break;
        case 0x2:
            if (cpu.get_id())
                printf("MOV {%d}, {%d}", destination, source);
            if (destination == REG_PC)
                cpu.jp(cpu.get_register(source), false);
            else
                cpu.mov(destination, cpu.get_register(source), false);
            break;
        case 0x3:
            if (high_dest)
            {
                if (cpu.get_id())
                    printf("BLX {%d}", source);
                cpu.set_register(REG_LR, cpu.get_PC() - 1);
            }
            else
            {
                if (cpu.get_id())
                    printf("BX {%d}", source);
            }
            cpu.jp(cpu.get_register(source), true);
            break;
        default:
            printf("High-reg Thumb opcode $%02X not recognized\n", opcode);
    }
}

void Interpreter::thumb_pc_rel_load(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = (instruction >> 8) & 0x7;
    uint32_t address = cpu.get_PC() + ((instruction & 0xFF) << 2);
    address &= ~0x3; //keep memory aligned safely
    
    if (cpu.get_id())
        printf("LDR {%d}, $%08X", destination, address);
    
    cpu.add_n32_data(address, 1);
    cpu.add_internal_cycles(1);
    cpu.set_register(destination, cpu.read_word(address));
}

void Interpreter::thumb_store_reg_offset(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    bool is_byte = (instruction & (1 << 10)) != 0;
    
    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t source = instruction & 0x7;
    uint32_t offset = (instruction >> 6) & 0x7;
    
    uint32_t address = cpu.get_register(base);
    address += cpu.get_register(offset);
    
    uint32_t source_contents = cpu.get_register(source);
    
    if (is_byte)
    {
        if (cpu.get_id())
            printf("STRB {%d}, [{%d}, {%d}]", source, base, offset);
        cpu.add_n16_data(address, 1);
        cpu.write_byte(address, source_contents & 0xFF);
    }
    else
    {
        if (cpu.get_id())
            printf("STR {%d}, [{%d}, {%d}]", source, base, offset);
        cpu.add_n32_data(address, 1);
        cpu.write_word(address, source_contents);
    }
}

void Interpreter::thumb_load_reg_offset(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    bool is_byte = (instruction & (1 << 10)) != 0;
    
    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t destination = instruction & 0x7;
    uint32_t offset = (instruction >> 6) & 0x7;
    
    uint32_t address = cpu.get_register(base);
    address += cpu.get_register(offset);
    
    if (is_byte)
    {
        if (cpu.get_id())
            printf("LDRB {%d}, [{%d}, {%d}]", destination, base, offset);
        cpu.add_n16_data(address, 1);
        cpu.add_internal_cycles(1);
        cpu.set_register(destination, cpu.read_byte(address));
    }
    else
    {
        if (cpu.get_id())
            printf("LDR {%d}, [{%d}, {%d}]", destination, base, offset);
        cpu.add_n32_data(address, 1);
        cpu.add_internal_cycles(1);
        uint32_t word = cpu.rotr32(cpu.read_word(address & ~0x3), (address & 0x3) * 8, false);
        cpu.set_register(destination, word);
    }
}

void Interpreter::thumb_store_halfword(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t offset = ((instruction >> 6) & 0x1F) << 1;
    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t source = instruction & 0x7;
    
    uint32_t address = cpu.get_register(base) + offset;
    uint32_t value = cpu.get_register(source) & 0xFFFF;
    
    if (cpu.get_id())
        printf("STRH {%d}, [{%d}, $%04X]", source, base, offset);
    
    cpu.add_n16_data(address, 1);
    cpu.write_halfword(address, value);
}

void Interpreter::thumb_load_halfword(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t offset = ((instruction >> 6) & 0x1F) << 1;
    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t destination = instruction & 0x7;
    
    uint32_t address = cpu.get_register(base) + offset;
    
    if (cpu.get_id())
        printf("LDRH {%d}, [{%d}, $%04X]", destination, base, offset);
    
    cpu.add_n16_data(address, 1);
    cpu.set_register(destination, cpu.read_halfword(address));
}

void Interpreter::thumb_store_imm_offset(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t source = instruction & 0x7;
    uint32_t base = (instruction >> 3) & 0x7;
    int offset = (instruction >> 6) & 0x1F;
    bool is_byte = instruction & (1 << 12);
    
    uint32_t address = cpu.get_register(base);
    
    if (is_byte)
    {
        address += offset;
        if (cpu.get_id())
            printf("STRB {%d}, [{%d}, $%02X]", source, base, offset);
        cpu.add_n16_data(address, 1);
        cpu.write_byte(address, cpu.get_register(source) & 0xFF);
    }
    else
    {
        offset <<= 2;
        address += offset;
        if (cpu.get_id())
            printf("STR {%d}, [{%d}, $%02X]", source, base, offset);
        cpu.add_n32_data(address, 1);
        cpu.write_word(address, cpu.get_register(source));
    }
}

void Interpreter::thumb_load_imm_offset(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = instruction & 0x7;
    uint32_t base = (instruction >> 3) & 0x7;
    int offset = (instruction >> 6) & 0x1F;
    bool is_byte = instruction & (1 << 12);
    
    uint32_t address = cpu.get_register(base);
    
    if (is_byte)
    {
        address += offset;
        if (cpu.get_id())
            printf("LDRB {%d}, [{%d}, $%02X]", destination, base, offset);
        cpu.add_n16_data(address, 1);
        cpu.set_register(destination, cpu.read_byte(address));
    }
    else
    {
        offset <<= 2;
        address += offset;
        if (cpu.get_id())
            printf("LDR {%d}, [{%d}, $%02X]", destination, base, offset);
        cpu.add_n32_data(address, 1);
        uint32_t word = cpu.rotr32(cpu.read_word(address & ~0x3), (address & 0x3) * 8, false);
        cpu.set_register(destination, word);
    }
    cpu.add_internal_cycles(1);
}

void Interpreter::thumb_load_store_sign_halfword(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = instruction & 0x7;
    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t offset = (instruction >> 6) & 0x7;
    int opcode = (instruction >> 10) & 0x3;
    
    uint32_t address = cpu.get_register(base);
    address += cpu.get_register(offset);
    
    switch (opcode)
    {
        case 0:
            if (cpu.get_id())
                printf("STRH {%d}, [{%d}, {%d}]", destination, base, offset);
            cpu.write_halfword(address, cpu.get_register(destination) & 0xFFFF);
            cpu.add_n32_data(address, 1);
            break;
        case 1:
            if (cpu.get_id())
                printf("LDSB {%d}, [{%d}, {%d}]", destination, base, offset);
        {
            uint32_t extended_byte = cpu.read_byte(address);
            extended_byte = (int32_t)(int8_t)extended_byte;
            cpu.set_register(destination, extended_byte);
            cpu.add_internal_cycles(1);
            cpu.add_n16_data(address, 1);
        }
            break;
        case 2:
            if (cpu.get_id())
                printf("LDRH {%d}, [{%d}, {%d}]", destination, base, offset);
            cpu.set_register(destination, cpu.read_halfword(address));
            cpu.add_n16_data(address, 1);
            cpu.add_internal_cycles(1);
            break;
        case 3:
            if (cpu.get_id())
                printf("LDSH {%d}, [{%d}, {%d}]", destination, base, offset);
        {
            uint32_t extended_halfword = cpu.read_halfword(address);
            extended_halfword = (int32_t)(int16_t)extended_halfword;
            cpu.set_register(destination, extended_halfword);
            cpu.add_internal_cycles(1);
            cpu.add_n16_data(address, 1);
        }
            break;
        default:
            printf("\nSign extended opcode %d not recognized", opcode);
    }
}

void Interpreter::thumb_sp_rel_store(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t source = (instruction >> 8) & 0x7;
    uint32_t offset = (instruction & 0xFF) << 2;
    
    uint32_t address = cpu.get_register(REG_SP);
    address += offset;
    
    //if (cpu.can_disassemble())
        //printf("STR {%d}, [{SP}, $%04X]", source, offset);
    
    cpu.add_n32_data(address, 1);
    cpu.write_word(address, cpu.get_register(source));
}

void Interpreter::thumb_sp_rel_load(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = (instruction >> 8) & 0x7;
    uint32_t offset = (instruction & 0xFF) << 2;
    
    uint32_t address = cpu.get_register(REG_SP);
    address += offset;
    
    //if (cpu.can_disassemble())
        //printf("LDR {%d}, [{SP}, $%04X]", destination, offset);
    
    cpu.add_n32_data(address, 1);
    cpu.add_internal_cycles(1);
    cpu.set_register(destination, cpu.read_word(address));
}

void Interpreter::thumb_offset_sp(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    int16_t offset = (instruction & 0x7F) << 2;
    
    if (instruction & (1 << 7))
        offset = -offset;
    
    uint32_t sp = cpu.get_register(REG_SP);
    cpu.set_register(REG_SP, sp + offset);
    
    //if (cpu.can_disassemble())
        //printf("ADD {SP}, $%04X", offset);
}

void Interpreter::thumb_load_address(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t destination = (instruction >> 8) & 0x7;
    uint32_t offset = (instruction & 0xFF) << 2;
    bool adding_SP = instruction & (1 << 11);
    
    uint32_t address;
    if (adding_SP)
        address = cpu.get_register(REG_SP);
    else
        address = cpu.get_PC() & ~0x2; //Set bit 1 to zero for alignment safety
    
    //if (cpu.can_disassemble())
        //printf("ADD {%d}, $%08X", destination, address);
    
    cpu.add(destination, address, offset, false);
}

void Interpreter::thumb_push(ARM_CPU& cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    int reg_list = instruction & 0xFF;
    uint32_t stack_pointer = cpu.get_register(REG_SP);
    
    //if (cpu.can_disassemble())
        //printf("PUSH $%02X", reg_list);
    
    int regs = 0;
    if (instruction & (1 << 8))
    {
        regs++;
        stack_pointer -= 4;
        cpu.write_word(stack_pointer, cpu.get_register(REG_LR));
    }
    
    for (int i = 7; i >= 0; i--)
    {
        int bit = 1 << i;
        if (reg_list & bit)
        {
            regs++;
            stack_pointer -= 4;
            cpu.write_word(stack_pointer, cpu.get_register(i));
        }
    }
    
    cpu.add_n32_data(stack_pointer, 2);
    if (regs > 2)
        cpu.add_s32_data(stack_pointer, regs - 2);
    
    cpu.set_register(REG_SP, stack_pointer);
}

void Interpreter::thumb_pop(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    int reg_list = instruction & 0xFF;
    uint32_t stack_pointer = cpu.get_register(REG_SP);
    
    //if (cpu.can_disassemble())
        //printf("POP $%02X", reg_list);
    
    int regs = 0;
    for (int i = 0; i < 8; i++)
    {
        int bit = 1 << i;
        if (reg_list & bit)
        {
            regs++;
            cpu.set_register(i, cpu.read_word(stack_pointer));
            stack_pointer += 4;
        }
    }
    
    if (instruction & (1 << 8))
    {
        //Only ARM9 can change thumb state by popping PC off the stack
        bool change_thumb_state = !cpu.get_id();
        cpu.jp(cpu.read_word(stack_pointer), change_thumb_state);
        
        regs++;
        stack_pointer += 4;
    }
    
    if (regs > 1)
        cpu.add_s32_data(stack_pointer, regs - 1);
    cpu.add_n32_data(stack_pointer, 1);
    cpu.add_internal_cycles(1);
    cpu.set_register(REG_SP, stack_pointer);
}

void Interpreter::thumb_store_multiple(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint8_t reg_list = instruction & 0xFF;
    uint32_t base = (instruction >> 8) & 0x7;
    
    uint32_t address = cpu.get_register(base);
    
    //if (cpu.can_disassemble())
        //printf("STMIA {%d}, $%02X", base, reg_list);
    
    int regs = 0;
    for (int reg = 0; reg < 8; reg++)
    {
        int bit = 1 << reg;
        if (reg_list & bit)
        {
            regs++;
            cpu.write_word(address, cpu.get_register(reg));
            address += 4;
        }
    }
    
    cpu.add_n32_data(address, 2);
    if (regs > 2)
        cpu.add_s32_data(address, regs - 2);
    cpu.set_register(base, address);
}

void Interpreter::thumb_load_multiple(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint8_t reg_list = instruction & 0xFF;
    uint32_t base = (instruction >> 8) & 0x7;
    
    uint32_t address = cpu.get_register(base);
    
    //if (cpu.can_disassemble())
        //printf("LDMIA {%d}, $%02X", base, reg_list);
    
    int regs = 0;
    for (int reg = 0; reg < 8; reg++)
    {
        int bit = 1 << reg;
        if (reg_list & bit)
        {
            regs++;
            cpu.set_register(reg, cpu.read_word(address));
            address += 4;
        }
    }
    
    cpu.add_n32_data(address, 2);
    if (regs > 1)
        cpu.add_s32_data(address, regs - 2);
    cpu.add_internal_cycles(1);
    
    int base_bit = 1 << base;
    if (!(reg_list & base_bit))
        cpu.set_register(base, address);
}

void Interpreter::thumb_branch(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t address = cpu.get_PC();
    int16_t offset = (instruction & 0x7FF) << 1;
    
    //Sign extend 12-bit offset
    offset <<= 4;
    offset >>= 4;
    
    address += offset;
    
    //if (cpu.can_disassemble())
        //printf("B $%04X", address);
    cpu.jp(address, false);
}

void Interpreter::thumb_cond_branch(ARM_CPU& cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    int condition = (instruction >> 8) & 0xF;
    if (condition == 0xF)
    {
        //if (cpu.can_disassemble())
            //printf("SWI $%02X", instruction & 0xFF);
        cpu.handle_SWI();
        return;
    }
    
    uint32_t address = cpu.get_PC();
    int16_t offset = static_cast<int32_t>(instruction << 24) >> 23;
    
    /*if (cpu.can_disassemble())
    {
        printf("B");
        cpu.print_condition(condition);
        printf(" $%08X", address + offset);
    }*/
    
    if (cpu.check_condition(condition))
        cpu.jp(address + offset, false);
}

void Interpreter::thumb_long_branch_prep(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t upper_address = cpu.get_PC();
    int32_t offset = ((instruction & 0x7FF) << 21) >> 9;
    upper_address += offset;
    
    //if (cpu.can_disassemble())
        //printf("BLP: $%08X", upper_address);
    
    cpu.set_register(REG_LR, upper_address);
}

void Interpreter::thumb_long_branch(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t address = cpu.get_register(REG_LR);
    address += (instruction & 0x7FF) << 1;
    
    uint32_t new_LR = cpu.get_PC() - 2;
    new_LR |= 0x1; //Preserve Thumb mode
    
    //if (cpu.can_disassemble())
        //printf("BL: $%08X", address);
    
    cpu.set_register(REG_LR, new_LR);
    
    cpu.jp(address, false);
}

void Interpreter::thumb_long_blx(ARM_CPU &cpu)
{
    uint16_t instruction = cpu.get_current_instr();
    uint32_t address = cpu.get_register(REG_LR);
    address += (instruction & 0x7FF) << 1;
    
    uint32_t new_LR = cpu.get_PC() - 2;
    new_LR |= 0x1; //Preserve Thumb mode
    
    //if (cpu.can_disassemble())
        //printf("BLX: $%08X", address);
    
    cpu.set_register(REG_LR, new_LR);
    
    cpu.jp(address, true); //Switch to ARM mode
}
