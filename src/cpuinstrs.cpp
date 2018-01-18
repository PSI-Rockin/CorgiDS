/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include <cstdlib>
#include "config.hpp"
#include "cpuinstrs.hpp"
#include "disassembler.hpp"
#include "instrtable.h"

//TODO: add instruction cycle timing for multiply/multiply long, as well as anything else I missed

void Interpreter::arm_interpret(ARM_CPU &cpu)
{
    uint32_t instruction = cpu.get_current_instr();
    int condition = (instruction & 0xF0000000) >> 28;
    
    if (cpu.get_id() && Config::test)
    {
        printf("\n");
        uint32_t PC = cpu.get_PC() - 8;
        if (!cpu.get_id())
            printf("(9A)");
        else
            printf("(7A)");
        printf("[$%08X] {$%08X} - ", PC, instruction);
        printf("%s", Disassembler::disasm_arm(cpu, instruction, PC).c_str());
    }
    
    uint32_t op = ((instruction >> 4) & 0xF) | ((instruction >> 16) & 0xFF0);
    //ARM_INSTR opcode = arm_decode(instruction);

    if (condition == 15 && (instruction & 0xFE000000) == 0xFA000000 && !cpu.get_id())
    {
        blx(cpu, instruction);
    }
    else
    {
        //printf("\n op: $%04X", op);
        if (cpu.check_condition(condition))
            arm_table[op](cpu, instruction);
    }
}

void Interpreter::undefined(ARM_CPU &cpu, uint32_t instruction)
{
    printf("\nUnrecognized ARM opcode $%08X", instruction);
    cpu.handle_UNDEFINED();
}

ARM_INSTR Interpreter::arm_decode(uint32_t instruction)
{
    //Here be dragons
    if ((instruction & (0x0F000000)) >> 24 == 0xA)
        return ARM_INSTR::BRANCH;
    
    if ((instruction & (0x0F000000)) >> 24 == 0xB)
        return ARM_INSTR::BRANCH_WITH_LINK;
    
    if (((instruction >> 4) & 0x0FFFFFF) == 0x12FFF1)
        return ARM_INSTR::BRANCH_EXCHANGE;
    
    if (((instruction >> 4) & 0x0FFFFFF) == 0x12FFF3)
        return ARM_INSTR::BRANCH_LINK_EXCHANGE;

    //ARMv5TE-only instructions
    if (((instruction >> 16) & 0xFFF) == 0x16F)
    {
        if (((instruction >> 4) & 0xFF) == 0xF1)
            return ARM_INSTR::COUNT_LEADING_ZEROS;
    }

    if (((instruction >> 4) & 0xFF) == 0x05)
    {
        if (((instruction >> 24) & 0xF) == 0x1)
        {
            int op = (instruction >> 20) & 0xF;
            if (op == 0 || op == 2 || op == 4 || op == 6)
                return ARM_INSTR::SATURATED_OP;
        }
    }
    
    //stupid bullshit, like seriously, why can't ARM be simple
    if (((instruction >> 26) & 0x3) == 0)
    {
        if ((instruction & (1 << 25)) == 0)
        {
            if (((instruction >> 4) & 0xFF) == 0x9)
            {
                if (((instruction >> 23) & 0x1F) == 0x2 && (((instruction >> 20) & 0x3) == 0))
                    return ARM_INSTR::SWAP;
            }
            if ((instruction & (1 << 7)) != 0 && (instruction & (1 << 4)) == 0)
            {
                if ((instruction & (1 << 20)) == 0 && ((instruction >> 23) & 0x3) == 0x2)
                    return ARM_INSTR::SIGNED_HALFWORD_MULTIPLY;
            }
            if ((instruction & (1 << 7)) != 0 && (instruction & (1 << 4)) != 0)
            {
                if (((instruction >> 4) & 0xF) == 0x9)
                {
                    if (((instruction >> 22) & 0x3F) == 0)
                        return ARM_INSTR::MULTIPLY;
                    else if (((instruction >> 23) & 0x1F) == 1)
                        return ARM_INSTR::MULTIPLY_LONG;
                    return ARM_INSTR::UNDEFINED;
                }
                else if ((instruction & (1 << 6)) == 0 && ((instruction & (1 << 5)) != 0))
                {
                    if ((instruction & (1 << 20)) != 0)
                        return ARM_INSTR::LOAD_HALFWORD;
                    else
                        return ARM_INSTR::STORE_HALFWORD;
                }
                else if ((instruction & (1 << 6)) != 0 && ((instruction & (1 << 5))) == 0)
                {
                    if ((instruction & (1 << 20)) != 0)
                        return ARM_INSTR::LOAD_SIGNED_BYTE;
                    else
                        return ARM_INSTR::LOAD_DOUBLEWORD;
                }
                else if ((instruction & (1 << 6)) != 0 && ((instruction & (1 << 5))) != 0)
                {
                    if ((instruction & (1 << 20)) != 0)
                        return ARM_INSTR::LOAD_SIGNED_HALFWORD;
                    else
                        return ARM_INSTR::STORE_DOUBLEWORD;
                }
                return ARM_INSTR::UNDEFINED;
            }
        }
        return ARM_INSTR::DATA_PROCESSING;
    }
    
    else if ((instruction & (0x0F000000)) >> 26 == 0x1)
    {
        if ((instruction & (1 << 20)) == 0)
        {
            if ((instruction & (1 << 22)) == 0)
                return ARM_INSTR::STORE_WORD;
            else
                return ARM_INSTR::STORE_BYTE;
        }
        else
        {
            if ((instruction & (1 << 22)) == 0)
                return ARM_INSTR::LOAD_WORD;
            else
                return ARM_INSTR::LOAD_BYTE;
        }
    }
    else if (((instruction >> 25) & 0x7) == 0x4)
    {
        if ((instruction & (1 << 20)) == 0)
            return ARM_INSTR::STORE_BLOCK;
        else
            return ARM_INSTR::LOAD_BLOCK;
    }
    else if (((instruction >> 24) & 0xF) == 0xE)
    {
        if ((instruction & (1 << 4)) != 0)
            return ARM_INSTR::COP_REG_TRANSFER;
        else
            return ARM_INSTR::COP_DATA_OP;
    }
    
    else if (((instruction >> 24) & 0xF) == 0xF)
        return ARM_INSTR::SWI;

    return ARM_INSTR::UNDEFINED;
}

uint32_t Interpreter::load_store_shift_reg(ARM_CPU& cpu, uint32_t instruction)
{
    int reg = cpu.get_register(instruction & 0xF);
    int shift_type = (instruction >> 5) & 0x3;
    int shift = (instruction >> 7) & 0x1F;
    
    switch (shift_type)
    {
        case 0: //Logical shift left
            reg = cpu.lsl(reg, shift, false);
            break;
        case 1: //Logical shift right
            if (!shift)
                shift = 32;
            reg = cpu.lsr(reg, shift, false);
            break;
        case 2: //Arithmetic shift right
            if (!shift)
                shift = 32;
            reg = cpu.asr(reg, shift, false);
            break;
        case 3: //Rotate right
            if (!shift)
                reg = cpu.rrx(reg, false);
            else
                reg = cpu.rotr32(reg, shift, false);
            break;
        default:
            printf("Invalid load/store shift: %d", shift_type);
            throw "[ARM_INSTR] Invalid load/store shift";
    }
    
    return reg;
}

void Interpreter::data_processing(ARM_CPU &cpu, uint32_t instruction)
{
    int opcode = (instruction >> 21) & 0xF;
    int first_operand = (instruction >> 16) & 0xF;
    uint32_t first_operand_contents = cpu.get_register(first_operand);
    bool set_condition_codes = instruction & (1 << 20);
    
    int destination = (instruction >> 12) & 0xF;
    uint32_t second_operand;
    
    bool is_operand_imm = instruction & (1 << 25);
    bool set_carry;
    int shift;
    
    switch (opcode)
    {
        case 0x0:
        case 0x1:
        case 0x8:
        case 0x9:
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
            set_carry = set_condition_codes;
            break;
        default:
            set_carry = false;
            break;
    }
    
    if (is_operand_imm)
    {
        //Immediate values are rotated right
        second_operand = instruction & 0xFF;
        //shift = (instruction >> 8) & 0xF;
        shift = (instruction & 0xF00) >> 7;
        
        second_operand = cpu.rotr32(second_operand, shift, set_carry);
    }
    else
    {
        second_operand = instruction & 0xF;
        second_operand = cpu.get_register(second_operand);
        int shift_type = (instruction >> 5) & 0x3;
        
        if (instruction & (1 << 4))
        {
            shift = cpu.get_register((instruction >> 8) & 0xF);
            cpu.add_internal_cycles(1); //Extra cycle due to SHIFT(reg) operation
            if ((instruction & 0xF) == REG_PC)
                second_operand = cpu.get_PC() + 4;
        }
        else
            shift = (instruction >> 7) & 0x1F;
        
        switch (shift_type)
        {
            case 0: //Logical shift left
                second_operand = cpu.lsl(second_operand, shift, set_carry);
                break;
            case 1: //Logical shift right
                if (shift || (instruction & (1 << 4)))
                    second_operand = cpu.lsr(second_operand, shift, set_carry);
                else
                    second_operand = cpu.lsr_32(second_operand, set_carry);
                break;
            case 2: //Arithmetic shift right
                if (shift || (instruction & (1 << 4)))
                    second_operand = cpu.asr(second_operand, shift, set_carry);
                else
                    second_operand = cpu.asr_32(second_operand, set_carry);
                break;
            case 3: //Rotate right
                if (shift == 0)
                    second_operand = cpu.rrx(second_operand, set_carry);
                else
                    second_operand = cpu.rotr32(second_operand, shift, set_carry);
                break;
            default:
                printf("\nInvalid data processing shift: %d", shift_type);
                throw "[ARM_INSTR] Invalid data processing shift";
        }
    }
    
    switch (opcode)
    {
        case 0x0:
            //if (!cpu.get_id())
                //printf("AND {%d}, {%d}, $%08X", destination, first_operand, second_operand);
            cpu.andd(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0x1:
            //if (!cpu.get_id())
                //printf("EOR {%d}, {%d}, $%08X", destination, first_operand, second_operand);
            cpu.eor(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0x2:
            //if (!cpu.get_id())
                //printf("SUB {%d}, {%d}, $%08X", destination, first_operand, second_operand);
            cpu.sub(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0x3:
            //Same as SUB, but switch the order of the operands
            //if (!cpu.get_id())
                //printf("RSB {%d}, $%08X, {%d}", destination, second_operand, first_operand);
            cpu.sub(destination, second_operand, first_operand_contents, set_condition_codes);
            break;
        case 0x4:
            //if (!cpu.get_id())
                //printf("ADD {%d}, {%d}, $%08X", destination, first_operand, second_operand);
            cpu.add(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0x5:
            //if (!cpu.get_id())
                //printf("ADC {%d}, {%d}, $%08X", destination, first_operand, second_operand);
            cpu.adc(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0x6:
            //if (!cpu.get_id())
                //printf("SBC {%d}, {%d}, $%08X", destination, first_operand, second_operand);
            cpu.sbc(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0x7:
            //if (!cpu.get_id())
                //printf("RSC {%d}, $%08X, {%d}", destination, second_operand, first_operand);
            cpu.sbc(destination, second_operand, first_operand_contents, set_condition_codes);
            break;
        case 0x8:
            if (set_condition_codes)
            {
                //if (!cpu.get_id())
                    //printf("TST {%d}, $%08X", first_operand, second_operand);
                cpu.tst(first_operand_contents, second_operand);
            }
            else
                cpu.mrs(instruction);
            break;
        case 0x9:
            if (set_condition_codes)
            {
                //if (!cpu.get_id())
                    //printf("TEQ {%d}, $%08X", first_operand, second_operand);
                cpu.teq(first_operand_contents, second_operand);
            }
            else
                cpu.msr(instruction);
            break;
        case 0xA:
            if (set_condition_codes)
            {
                //if (!cpu.get_id())
                    //printf("CMP {%d}, $%08X", first_operand, second_operand);
                cpu.cmp(first_operand_contents, second_operand);
            }
            else
                cpu.mrs(instruction);
            break;
        case 0xB:
            if (set_condition_codes)
            {
                //if (!cpu.get_id())
                    //printf("CMN {%d}, $%08X", first_operand, second_operand);
                cpu.cmn(first_operand_contents, second_operand);
            }
            else
                cpu.msr(instruction);
            break;
        case 0xC:
            //if (!cpu.get_id())
                //printf("ORR {%d}, {%d}, $%08X", destination, first_operand, second_operand);
            cpu.orr(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0xD:
            //if (!cpu.get_id())
                //printf("MOV {%d}, $%08X", destination, second_operand);
            cpu.mov(destination, second_operand, set_condition_codes);
            break;
        case 0xE:
            //if (!cpu.get_id())
                //printf("BIC {%d}, $%08X", destination, second_operand);
            cpu.bic(destination, first_operand_contents, second_operand, set_condition_codes);
            break;
        case 0xF:
            //if (!cpu.get_id())
                //printf("MVN {%d}, $%08X", destination, second_operand);
            cpu.mvn(destination, second_operand, set_condition_codes);
            break;
        default:
            printf("Data processing opcode $%01X not recognized\n", opcode);
    }
}

void Interpreter::count_leading_zeros(ARM_CPU &cpu, uint32_t instruction)
{
    if (cpu.get_id())
    {
        cpu.handle_UNDEFINED();
        return;
    }
    uint32_t source = instruction & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;

    //if (cpu.can_disassemble())
        //printf("CLZ {%d}, {%d}", destination, source);

    source = cpu.get_register(source);

    //Implementation from melonDS (I thought mine was wrong, but apparently it's correct)
    //Regardless I'm too lazy to change it, especially when my initial implementation was worse
    uint32_t bits = 0;
    while ((source & 0xFF000000) == 0)
    {
        bits += 8;
        source <<= 8;
        source |= 0xFF;
    }

    while ((source & 0x80000000) == 0)
    {
        bits++;
        source <<= 1;
        source |= 1;
    }

    cpu.set_register(destination, bits);
}

void Interpreter::saturated_op(ARM_CPU &cpu, uint32_t instruction)
{
    if (cpu.get_id())
    {
        cpu.handle_UNDEFINED();
        return;
    }

    int opcode = (instruction >> 20) & 0xF;
    uint32_t operand = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    uint32_t source = instruction & 0xF;

    uint32_t operand_reg = cpu.get_register(operand);
    uint32_t source_reg = cpu.get_register(source);
    uint32_t result;

    switch (opcode)
    {
        case 0:
            //QADD
            result = operand_reg + source_reg;
            if (ADD_OVERFLOW(source_reg, operand_reg, result))
            {
                cpu.get_CPSR()->sticky_overflow = true;
                if (result & (0x80000000))
                    result = 0x7FFFFFFF;
                else
                    result = 0x80000000;
            }
            cpu.set_register(destination, result);
            break;
        case 0x2:
            //QSUB
            result = source_reg - operand_reg;
            if (SUB_OVERFLOW(source_reg, operand_reg, result))
            {
                cpu.get_CPSR()->sticky_overflow = true;
                if (result & (0x80000000))
                    result = 0x7FFFFFFF;
                else
                    result = 0x80000000;
            }
            cpu.set_register(destination, result);
            break;
        /*case 0x4:
            //QDADD
            operand = cpu.get_register(operand) * 2;
            result = cpu.get_register(source) + operand;
            if (result > 0xFFFFFFFF)
            {
                cpu.get_CPSR()->sticky_overflow = true;
                cpu.set_register(destination, 0xFFFFFFFF);
            }
            else
                cpu.set_register(destination, result);
            break;
        case 0x6:
            //QDSUB
            operand = cpu.get_register(operand) * 2;
            result = cpu.get_register(source) - operand;
            if (result > source)
            {
                cpu.get_CPSR()->sticky_overflow = true;
                cpu.set_register(destination, 0);
            }
            else
                cpu.set_register(destination, result);
            break;*/
        default:
            printf("\nUnrecognized saturated opcode %d", opcode);
            throw "[ARM_INSTR] Unrecognized sat op";
    }
}

void Interpreter::multiply(ARM_CPU &cpu, uint32_t instruction)
{
    bool accumulate = instruction & (1 << 21);
    bool set_condition_codes = instruction & (1 << 20);
    int destination = (instruction >> 16) & 0xF;
    int first_operand = instruction & 0xF;
    int second_operand = (instruction >> 8) & 0xF;
    int third_operand = (instruction >> 12) & 0xF;
    
    uint32_t result = cpu.get_register(first_operand) * cpu.get_register(second_operand);
    
    if (accumulate)
    {
        //if (cpu.can_disassemble())
            //printf("MLA {%d}, {%d}, {%d}, {%d}", destination, first_operand, second_operand, third_operand);
        result += cpu.get_register(third_operand);
    }
    //else if (cpu.can_disassemble())
        //printf("MUL {%d}, {%d}, {%d}", destination, first_operand, second_operand);
    
    if (set_condition_codes)
    {
        cpu.set_zero_neg_flags(result);
    }
    
    cpu.set_register(destination, result);
}

void Interpreter::multiply_long(ARM_CPU &cpu, uint32_t instruction)
{
    bool is_signed = instruction & (1 << 22);
    bool accumulate = instruction & (1 << 21);
    bool set_condition_codes = instruction & (1 << 20);

    int dest_hi = (instruction >> 16) & 0xF;
    int dest_lo = (instruction >> 12) & 0xF;
    uint32_t first_operand = (instruction >> 8) & 0xF;
    uint32_t second_operand = (instruction) & 0xF;
    
    first_operand = cpu.get_register(first_operand);
    second_operand = cpu.get_register(second_operand);
    
    if (is_signed)
    {
        int64_t result = static_cast<int64_t>((int32_t)first_operand) * static_cast<int64_t>((int32_t)second_operand);
        
        if (accumulate)
        {
            int64_t big_reg = cpu.get_register(dest_lo);
            big_reg |= static_cast<int64_t>(cpu.get_register(dest_hi)) << 32;
            
            result += big_reg;
        }
        
        cpu.set_register(dest_lo, result & 0xFFFFFFFF);
        cpu.set_register(dest_hi, result >> 32ULL);
        
        if (set_condition_codes)
        {
            cpu.set_zero(!result);
            cpu.set_neg(result >> 63);
        }
    }
    else
    {
        uint64_t result = static_cast<uint64_t>(first_operand) * static_cast<uint64_t>(second_operand);
        
        if (accumulate)
        {
            uint64_t big_reg = cpu.get_register(dest_lo);
            big_reg |= static_cast<uint64_t>(cpu.get_register(dest_hi)) << 32;
            
            result += big_reg;
        }
        
        cpu.set_register(dest_lo, result & 0xFFFFFFFF);
        cpu.set_register(dest_hi, result >> 32ULL);
        
        if (set_condition_codes)
        {
            cpu.set_zero(!result);
            cpu.set_neg(result >> 63);
        }
    }
}

void Interpreter::signed_halfword_multiply(ARM_CPU &cpu, uint32_t instruction)
{
    //No op?
    if (cpu.get_id())
        return;
    uint32_t destination = (instruction >> 16) & 0xF;
    uint32_t accumulate = (instruction >> 12) & 0xF;
    uint32_t first_operand = (instruction >> 8) & 0xF;
    uint32_t second_operand = instruction & 0xF;
    int opcode = (instruction >> 21) & 0xF;

    bool first_op_top = instruction & (1 << 6);
    bool second_op_top = instruction & (1 << 5);

    uint32_t product;
    uint32_t result;

    switch (opcode)
    {
        case 0x8:
            //SMLAxy
            if (first_op_top)
                product = (int16_t)(cpu.get_register(first_operand) >> 16);
            else
                product = (int16_t)(cpu.get_register(first_operand) & 0xFFFF);

            if (second_op_top)
                product *= (int16_t)(cpu.get_register(second_operand) >> 16);
            else
                product *= (int16_t)(cpu.get_register(second_operand) & 0xFFFF);

            result = product + cpu.get_register(accumulate);

            if (ADD_OVERFLOW(product, cpu.get_register(accumulate), result))
                cpu.get_CPSR()->sticky_overflow = true;
            break;
        case 0x9:
            //SMULWy/SMLAWy
            //TODO: is all this correct?
            if (first_op_top)
                product = (int16_t)(cpu.get_register(first_operand) >> 16);
            else
                product = (int16_t)(cpu.get_register(first_operand) & 0xFFFF);

            if (!(instruction & (1 << 5))) //SMLAW
            {
                int64_t big_product = product * (int32_t)cpu.get_register(second_operand);
                big_product /= 0x10000;
                result = big_product + cpu.get_register(accumulate);

                if (ADD_OVERFLOW(big_product, cpu.get_register(accumulate), result))
                    cpu.get_CPSR()->sticky_overflow = true;
            }
            else //SMULW
            {
                int64_t big_product = product * (int32_t)cpu.get_register(second_operand);
                big_product /= 0x10000;
                result = big_product;
            }
            break;
        case 0xB:
            //SMULxy
            if (first_op_top)
                product = (int16_t)(cpu.get_register(first_operand) >> 16);
            else
                product = (int16_t)(cpu.get_register(first_operand) & 0xFFFF);

            if (second_op_top)
                product *= (int16_t)(cpu.get_register(second_operand) >> 16);
            else
                product *= (int16_t)(cpu.get_register(second_operand) & 0xFFFF);

            result = product;
            break;
        default:
            printf("\nUnrecognized smul opcode $%01X", opcode);
            throw "[ARM_INSTR] Unrecognized smul op";
    }

    cpu.set_register(destination, result);
}

void Interpreter::swap(ARM_CPU &cpu, uint32_t instruction)
{
    bool is_byte = instruction & (1 << 22);
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    uint32_t source = instruction & 0xF;
    
    uint32_t address = cpu.get_register(base);
    
    if (is_byte)
    {
        //if (cpu.can_disassemble())
            //printf("SWPB {%d}, {%d}, [{%d}]", destination, source, base);
        uint8_t byte = cpu.read_byte(address);
        cpu.write_byte(address, cpu.get_register(source) & 0xFF);
        cpu.set_register(destination, byte);
        
        cpu.add_n16_data(address, 2);
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("SWP {%d}, {%d}, [{%d}]", destination, source, base);
        uint32_t word = cpu.rotr32(cpu.read_word(address & ~0x3), (address & 0x3) * 8, false);
        cpu.write_word(address, cpu.get_register(source));
        cpu.set_register(destination, word);
        
        cpu.add_n32_data(address, 2);
    }
}

void Interpreter::store_word(ARM_CPU &cpu, uint32_t instruction)
{
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t source = (instruction >> 12) & 0xF;
    uint32_t offset;
    
    bool is_imm = (instruction & (1 << 25)) == 0;
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    
    if (is_imm)
        offset = instruction & 0xFFF;
    else
        offset = load_store_shift_reg(cpu, instruction);
    
    uint32_t address = cpu.get_register(base);
    uint32_t value = cpu.get_register(source);
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (is_writing_back)
            cpu.set_register(base, address);
        
        //if (cpu.get_id())
            //printf("STR {%d}, [{%d}, $%08X]", source, base, offset);
        cpu.add_n32_data(address, 1);
        cpu.write_word(address & ~0x3, value);
    }
    else
    {
        //if (cpu.get_id())
            //printf("STR {%d}, [{%d}, $%08X]", source, base, offset);
        cpu.add_n32_data(address, 1);
        cpu.write_word(address & ~0x3, value);
        
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        cpu.set_register(base, address);
    }
}

void Interpreter::load_word(ARM_CPU &cpu, uint32_t instruction)
{
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    uint32_t offset;
    
    bool is_imm = (instruction & (1 << 25)) == 0;
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    
    if (is_imm)
        offset = instruction & 0xFFF;
    else
        offset = load_store_shift_reg(cpu, instruction);
    
    uint32_t address = cpu.get_register(base);
    cpu.add_n32_data(address, 1);
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (is_writing_back)
            cpu.set_register(base, address);
        
        /*if (cpu.get_id())
        {
            if (is_adding_offset)
                printf("LDR {%d}, [{%d}, $%08X]", destination, base, offset);
            else
                printf("LDR {%d}, [{%d}, -$%08X]", destination, base, offset);
        }*/
        uint32_t word = cpu.rotr32(cpu.read_word(address & ~0x3), (address & 0x3) * 8, false);
        
        if (destination == REG_PC)
            cpu.jp(word, !cpu.get_id()); //Only ARM9 can change thumb state
        else
            cpu.set_register(destination, word);
    }
    else
    {
        //if (cpu.get_id())
            //printf("LDR {%d}, [{%d}], $%08X", destination, base, offset);
        
        uint32_t word = cpu.rotr32(cpu.read_word(address & ~0x3), (address & 0x3) * 8, false);
        
        if (destination == REG_PC)
            cpu.jp(word, !cpu.get_id()); //Only ARM9 can change thumb state
        else
            cpu.set_register(destination, word);
        
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (base != destination)
            cpu.set_register(base, address);
    }
}

void Interpreter::store_byte(ARM_CPU &cpu, uint32_t instruction)
{
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t source = (instruction >> 12) & 0xF;
    uint32_t offset;
    
    bool is_imm = (instruction & (1 << 25)) == 0;
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    
    if (is_imm)
        offset = instruction & 0xFFF;
    else
        offset = load_store_shift_reg(cpu, instruction);
    
    uint32_t address = cpu.get_register(base);
    uint8_t value = cpu.get_register(source) & 0xFF;
    
    cpu.add_n16_data(address, 1);
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (is_writing_back)
            cpu.set_register(base, address);
        
        //if (cpu.can_disassemble())
            //printf("STRB {%d}, [{%d}, $%08X]", source, base, offset);
        cpu.write_byte(address, value);
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("STRB {%d}, [{%d}], $%08X", source, base, offset);
        cpu.write_byte(address, value);
        
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        cpu.set_register(base, address);
    }
}

void Interpreter::load_byte(ARM_CPU &cpu, uint32_t instruction)
{
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    uint32_t offset;
    
    bool is_imm = (instruction & (1 << 25)) == 0;
    bool is_preindexing = instruction & (1 << 24);
    bool is_adding_offset = instruction & (1 << 23);
    bool is_writing_back = instruction & (1 << 21);
    
    if (is_imm)
        offset = instruction & 0xFFF;
    else
        offset = load_store_shift_reg(cpu, instruction);
    
    uint32_t address = cpu.get_register(base);
    cpu.add_n16_data(address, 1);
    cpu.add_internal_cycles(1);
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (is_writing_back)
            cpu.set_register(base, address);
        
        //if (cpu.can_disassemble())
            //printf("LDRB {%d}, [{%d}, $%08X]", destination, base, offset);
        cpu.set_register(destination, cpu.read_byte(address));
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("LDRB {%d}, [{%d}], $%08X", destination, base, offset);
        cpu.set_register(destination, cpu.read_byte(address));
        
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (base != destination)
            cpu.set_register(base, address);
    }
}

void Interpreter::store_halfword(ARM_CPU &cpu, uint32_t instruction)
{
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t source = (instruction >> 12) & 0xF;
    
    bool is_imm_offset = (instruction & (1 << 22)) != 0;
    uint32_t offset = 0;
    
    offset = instruction & 0xF;
    if (is_imm_offset)
        offset |= (instruction >> 4) & 0xF0;
    else
        offset = cpu.get_register(offset);
    
    uint32_t address = cpu.get_register(base);
    uint16_t halfword = cpu.get_register(source) & 0xFFFF;
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        //if (cpu.can_disassemble())
            //printf("STRH {%d}, $%08X", source, address);
        cpu.write_halfword(address, halfword);
        if (is_writing_back)
            cpu.set_register(base, address);
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("STRH {%d}, $%08X", source, address);
        cpu.write_halfword(address, halfword);
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        cpu.set_register(base, address);
    }
    cpu.add_n16_data(address, 1);
}

void Interpreter::load_halfword(ARM_CPU &cpu, uint32_t instruction)
{
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    
    bool is_imm_offset = (instruction & (1 << 22)) != 0;
    uint32_t offset = 0;
    
    offset = instruction & 0xF;
    if (is_imm_offset)
        offset |= (instruction >> 4) & 0xF0;
    else
        offset = cpu.get_register(offset);
    
    uint32_t address = cpu.get_register(base);
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (is_writing_back && base != destination)
            cpu.set_register(base, address);
        
        //if (cpu.can_disassemble())
            //printf("LDRH {%d}, $%08X", destination, address);
        cpu.set_register(destination, cpu.read_halfword(address));
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("LDRH {%d}, $%08X", destination, address);
        cpu.set_register(destination, cpu.read_halfword(address));
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (base != destination)
            cpu.set_register(base, address);
    }
    
    cpu.add_n16_data(address, 1);
}

void Interpreter::load_signed_byte(ARM_CPU &cpu, uint32_t instruction)
{
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    
    bool is_imm_offset = (instruction & (1 << 22)) != 0;
    uint32_t offset = 0;
    
    offset = instruction & 0xF;
    if (is_imm_offset)
        offset |= (instruction >> 4) & 0xF0;
    else
        offset = cpu.get_register(offset);
    
    uint32_t address = cpu.get_register(base);
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (is_writing_back)
            cpu.set_register(base, address);
        
        //if (cpu.can_disassemble())
            //printf("LDRSB {%d}, $%08X", destination, address);
        uint32_t word = static_cast<int32_t>(static_cast<int8_t>(cpu.read_byte(address)));
        cpu.set_register(destination, word);
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("LDRSB {%d}, $%08X", destination, address);
        uint32_t word = static_cast<int32_t>(static_cast<int8_t>(cpu.read_byte(address)));
        cpu.set_register(destination, word);
        
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (base != destination)
            cpu.set_register(base, address);
    }
    
    cpu.add_n16_data(address, 1);
}

void Interpreter::load_signed_halfword(ARM_CPU &cpu, uint32_t instruction)
{
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    
    bool is_imm_offset = (instruction & (1 << 22)) != 0;
    uint32_t offset = 0;
    
    offset = instruction & 0xF;
    if (is_imm_offset)
        offset |= (instruction >> 4) & 0xF0;
    else
        offset = cpu.get_register(offset);
    
    uint32_t address = cpu.get_register(base);
    
    if (is_preindexing)
    {
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (is_writing_back)
            cpu.set_register(base, address);
        
        //if (cpu.can_disassemble())
            //printf("LDRSB {%d}, $%08X", destination, address);
        uint32_t word = static_cast<int32_t>(static_cast<int16_t>(cpu.read_halfword(address)));
        cpu.set_register(destination, word);
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("LDRSB {%d}, $%08X", destination, address);
        uint32_t word = static_cast<int32_t>(static_cast<int16_t>(cpu.read_halfword(address)));
        cpu.set_register(destination, word);
        
        if (is_adding_offset)
            address += offset;
        else
            address -= offset;
        
        if (base != destination)
            cpu.set_register(base, address);
    }
    
    cpu.add_n16_data(address, 1);
}

void Interpreter::store_doubleword(ARM_CPU &cpu, uint32_t instruction)
{
    if (cpu.get_id())
    {
        cpu.handle_UNDEFINED();
        return;
    }

    bool is_preindexing = instruction & (1 << 24);
    bool add_offset = instruction & (1 << 23);
    bool is_imm_offset = instruction & (1 << 22);
    bool write_back = instruction & (1 << 21);

    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t source = (instruction >> 12) & 0xF;

    int offset;

    offset = instruction & 0xF;
    if (is_imm_offset)
        offset |= (instruction >> 4) & 0xF0;
    else
        offset = cpu.get_register(offset);

    uint32_t address = cpu.get_register(base);

    if (is_preindexing)
    {
        if (add_offset)
            address += offset;
        else
            address -= offset;

        cpu.write_word(address, cpu.get_register(source));
        cpu.write_word(address + 4, cpu.get_register(source + 1));

        if (write_back)
            cpu.set_register(base, address + 4);
    }
    else
    {
        printf("STRD postindexing not supported");
        throw "[ARM_INSTR] STD postindexing not supported";
    }
}

void Interpreter::load_doubleword(ARM_CPU &cpu, uint32_t instruction)
{
    if (cpu.get_id())
    {
        //nop
        return;
    }

    bool is_preindexing = instruction & (1 << 24);
    bool add_offset = instruction & (1 << 23);
    bool is_imm_offset = instruction & (1 << 22);
    bool write_back = instruction & (1 << 21);

    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t dest = (instruction >> 12) & 0xF;

    int offset;

    offset = instruction & 0xF;
    if (is_imm_offset)
        offset |= (instruction >> 4) & 0xF0;
    else
        offset = cpu.get_register(offset);

    uint32_t address = cpu.get_register(base);

    if (is_preindexing)
    {
        if (add_offset)
            address += offset;
        else
            address -= offset;

        cpu.set_register(dest, cpu.read_word(address));
        cpu.set_register(dest + 1, cpu.read_word(address + 4));

        if (write_back)
            cpu.set_register(base, address + 4);
    }
    else
    {
        cpu.set_register(dest, cpu.read_word(address));
        cpu.set_register(dest + 1, cpu.read_word(address + 4));

        if (add_offset)
            address += offset;
        else
            address -= offset;
        cpu.set_register(base, address);
    }
}

void Interpreter::store_block(ARM_CPU &cpu, uint32_t instruction)
{
    uint16_t reg_list = instruction & 0xFFFF;
    uint32_t base = (instruction >> 16) & 0xF;
    bool is_writing_back = instruction & (1 << 21);
    bool load_PSR = instruction & (1 << 22);
    bool is_adding_offset = instruction & (1 << 23);
    bool is_preindexing = instruction & (1 << 24);
    bool user_bank_transfer = load_PSR && !(reg_list & (1 << 15));
    
    uint32_t address = cpu.get_register(base);
    int offset;
    if (is_adding_offset)
        offset = 4;
    else
        offset = -4;
    
    /*if (true)
    {
        if (base == REG_SP && !is_adding_offset && is_preindexing && is_writing_back)
            printf("\nPUSH $%04X", reg_list);
        else
            printf("\nSTM {%d}, $%04X", base, reg_list);
        printf("\nPC: $%08X", cpu.get_register(REG_PC));
    }*/
    
    PSR_Flags* cpsr = cpu.get_CPSR();
    PSR_MODE old_mode = cpsr->mode;
    if (user_bank_transfer)
    {
        cpu.update_reg_mode(PSR_MODE::USER);
        cpsr->mode = PSR_MODE::USER;
    }
    
    int regs = 0;
    
    if (is_adding_offset)
    {
        for (int i = 0; i < 16; i++)
        {
            int bit = 1 << i;
            if (reg_list & bit)
            {
                regs++;
                if (is_preindexing)
                {
                    address += offset;
                    cpu.write_word(address, cpu.get_register(i));
                }
                else
                {
                    cpu.write_word(address, cpu.get_register(i));
                    address += offset;
                }
            }
        }
    }
    else
    {
        for (int i = 15; i >= 0; i--)
        {
            int bit = 1 << i;
            if (reg_list & bit)
            {
                regs++;
                if (is_preindexing)
                {
                    address += offset;
                    cpu.write_word(address, cpu.get_register(i));
                }
                else
                {
                    cpu.write_word(address, cpu.get_register(i));
                    address += offset;
                }
            }
        }
    }

    if (user_bank_transfer)
    {
        cpu.update_reg_mode(old_mode);
        cpsr->mode = old_mode;
    }
    
    if (regs > 2)
        cpu.add_s32_data(address, regs - 1);
    cpu.add_n32_data(address, 2);
    if (is_writing_back)
        cpu.set_register(base, address);
}

void Interpreter::load_block(ARM_CPU &cpu, uint32_t instruction)
{
    uint16_t reg_list = instruction & 0xFFFF;
    uint32_t base = (instruction >> 16) & 0xF;
    bool is_writing_back = instruction & (1 << 21);
    bool load_PSR = instruction & (1 << 22);
    bool is_adding_offset = instruction & (1 << 23);
    bool is_preindexing = instruction & (1 << 24);
    bool user_bank_transfer = load_PSR && !(reg_list & (1 << 15));
    
    uint32_t address = cpu.get_register(base);
    int offset;
    if (is_adding_offset)
        offset = 4;
    else
        offset = -4;
    
    /*if (cpu.can_disassemble())
    {
        if (base == REG_SP && is_adding_offset && !is_preindexing && is_writing_back)
            printf("POP $%04X", reg_list);
        else
            printf("LDM {%d}, $%04X", base, reg_list);
    }*/

    PSR_Flags* cpsr = cpu.get_CPSR();
    PSR_MODE old_mode = cpsr->mode;
    if (user_bank_transfer)
    {
        cpu.update_reg_mode(PSR_MODE::USER);
        cpsr->mode = PSR_MODE::USER;
    }
    
    int regs = 0;
    if (is_adding_offset)
    {
        for (int i = 0; i < 15; i++)
        {
            int bit = 1 << i;
            if (reg_list & bit)
            {
                regs++;
                if (is_preindexing)
                {
                    address += offset;
                    cpu.set_register(i, cpu.read_word(address));
                }
                else
                {
                    cpu.set_register(i, cpu.read_word(address));
                    address += offset;
                }
            }
        }
        if (reg_list & (1 << 15))
        {
            if (is_preindexing)
            {
                address += offset;
                uint32_t new_PC = cpu.read_word(address);
                if (cpu.get_id())
                    new_PC &= ~0x1;
                cpu.jp(new_PC, true);
            }
            else
            {
                uint32_t new_PC = cpu.read_word(address);
                if (cpu.get_id())
                    new_PC &= ~0x1;
                cpu.jp(new_PC, true);
                address += offset;
            }
            if (load_PSR)
                cpu.spsr_to_cpsr();
            regs++;
        }
    }
    else
    {
        if (reg_list & (1 << 15))
        {
            if (is_preindexing)
            {
                address += offset;
                uint32_t new_PC = cpu.read_word(address);
                if (cpu.get_id())
                    new_PC &= ~0x1;
                cpu.jp(new_PC, true);
            }
            else
            {
                uint32_t new_PC = cpu.read_word(address);
                if (cpu.get_id())
                    new_PC &= ~0x1;
                cpu.jp(new_PC, true);
                address += offset;
            }
            if (load_PSR)
                cpu.spsr_to_cpsr();
            regs++;
        }
        for (int i = 14; i >= 0; i--)
        {
            int bit = 1 << i;
            if (reg_list & bit)
            {
                regs++;
                if (is_preindexing)
                {
                    address += offset;
                    cpu.set_register(i, cpu.read_word(address));
                }
                else
                {
                    cpu.set_register(i, cpu.read_word(address));
                    address += offset;
                }
            }
        }
    }

    if (user_bank_transfer)
    {
        cpu.update_reg_mode(old_mode);
        cpsr->mode = old_mode;
    }
    
    if (regs > 1)
        cpu.add_s32_data(address, regs - 1);
    cpu.add_n32_data(address, 1);
    cpu.add_internal_cycles(1);
    if (is_writing_back && !((reg_list & (1 << base)) && cpu.get_id()))
        cpu.set_register(base, address);
}

void Interpreter::branch(ARM_CPU& cpu, uint32_t instruction)
{
    uint32_t address = cpu.get_PC();
    int32_t offset = (instruction & 0xFFFFFF) << 2;
    
    //Sign extend offset
    offset <<= 6;
    offset >>= 6;
    
    address += offset;
    
    //if (cpu.can_disassemble())
        //printf("B $%08X", address);
    
    cpu.jp(address, false);
}

void Interpreter::branch_link(ARM_CPU &cpu, uint32_t instruction)
{
    uint32_t address = cpu.get_PC();
    int32_t offset = (instruction & 0xFFFFFF) << 2;
    
    offset <<= 6;
    offset >>= 6;
    
    //if (cpu.can_disassemble())
        //printf("BL $%08X", address + offset);
    
    cpu.jp(address + offset, false);
    cpu.set_register(REG_LR, address - 4);
}

void Interpreter::branch_exchange(ARM_CPU &cpu, uint32_t instruction)
{
    uint32_t reg_id = instruction & 0xF;
    uint32_t new_address = cpu.get_register(reg_id);
    
    //if (cpu.get_id())
        //printf("BX {%d}", reg_id);
    
    cpu.jp(new_address, true);
}

void Interpreter::blx(ARM_CPU &cpu, uint32_t instruction)
{
    uint32_t address = cpu.get_PC();
    int32_t offset = (instruction & 0xFFFFFF) << 2;
    
    offset <<= 6;
    offset >>= 6;
    
    if (instruction & (1 << 24))
        offset += 2;
    
    //if (cpu.can_disassemble())
        //printf("BLX $%08X\n", address + offset);
    
    cpu.set_register(REG_LR, address - 4);
    cpu.jp(address + offset + 1, true);
}

void Interpreter::blx_reg(ARM_CPU &cpu, uint32_t instruction)
{
    cpu.set_register(REG_LR, cpu.get_PC() - 4);
    
    uint32_t reg_id = instruction & 0xF;
    
    //if (cpu.can_disassemble())
        //printf("BLX {%d}", reg_id);
    
    uint32_t new_address = cpu.get_register(reg_id);
    cpu.jp(new_address, true);
}

void Interpreter::coprocessor_reg_transfer(ARM_CPU &cpu, uint32_t instruction)
{
    CP15* cp15 = cpu.get_cp15();
    if (cp15 == nullptr)
    {
        //if (cpu.can_disassemble())
            //printf("[coprocessor op ignored]");
        return;
    }
    int operation_mode = (instruction >> 21) & 0x7;
    bool is_loading = (instruction & (1 << 20)) != 0;
    uint32_t CP_reg = (instruction >> 16) & 0xF;
    uint32_t ARM_reg = (instruction >> 12) & 0xF;
    
    int coprocessor_id = (instruction >> 8) & 0xF;
    int coprocessor_info = (instruction >> 5) & 0x7;
    int coprocessor_operand = instruction & 0xF;
    
    if (is_loading)
    {
        //if (cpu.can_disassemble())
            //printf("MRC %d, %d, {%d}, {%d}, {%d}, [%d]", coprocessor_id, operation_mode, ARM_reg, CP_reg, coprocessor_operand, coprocessor_info);
        cpu.add_internal_cycles(2);
        cpu.add_cop_cycles(1);
        switch (coprocessor_id)
        {
            case 15:
            {
                uint32_t value = cp15->mrc(operation_mode, CP_reg, coprocessor_info, coprocessor_operand);
                cpu.set_register(ARM_reg, value);
                break;
            }
            default:
                printf("Coprocessor %d not recognized", coprocessor_id);
                throw "[ARM_INSTR] Unrecognized MRC cop id";
        }
    }
    else
    {
        //if (cpu.can_disassemble())
            //printf("MCR %d, %d, {%d}, {%d}, {%d}, [%d]", coprocessor_id, operation_mode, ARM_reg, CP_reg, coprocessor_operand, coprocessor_info);
        cpu.add_internal_cycles(1);
        cpu.add_cop_cycles(1);
        switch (coprocessor_id)
        {
            case 15:
            {
                uint32_t value = cpu.get_register(ARM_reg);
                cp15->mcr(operation_mode, CP_reg, value, coprocessor_info, coprocessor_operand);
                break;
            }
            default:
                printf("Coprocessor %d not recognized", coprocessor_id);
                throw "[ARM_INSTR] Unrecognized MCR cop id";
        }
    }
}

void Interpreter::swi(ARM_CPU &cpu, uint32_t instruction)
{
    cpu.handle_SWI();
}
