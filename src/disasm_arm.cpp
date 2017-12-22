/*
    CorgiDS Copyright PSISP 2017
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <sstream>
#include "disassembler.hpp"

using namespace std;

string Disassembler::disasm_arm(ARM_CPU& cpu, uint32_t instruction, uint32_t address)
{
    ARM_INSTR opcode = Interpreter::arm_decode(instruction);

    string output = "";

    switch (opcode)
    {
        case ARM_INSTR::DATA_PROCESSING:
            output = disasm_data_processing(cpu, instruction);
            break;
        case ARM_INSTR::STORE_BYTE:
        case ARM_INSTR::LOAD_BYTE:
        case ARM_INSTR::STORE_WORD:
        case ARM_INSTR::LOAD_WORD:
            output = disasm_load_store(cpu, instruction);
            break;
        case ARM_INSTR::BRANCH:
        case ARM_INSTR::BRANCH_WITH_LINK:
            output = disasm_branch(cpu, instruction, address);
            break;
        case ARM_INSTR::BRANCH_EXCHANGE:
            output = disasm_bx(instruction);
            break;
        default:
            return "(UNDEFINED)";
    }

    return output;
}

string Disassembler::disasm_data_processing(ARM_CPU& cpu, uint32_t instruction)
{
    stringstream output;
    int opcode = (instruction >> 21) & 0xF;
    int first_operand = (instruction >> 16) & 0xF;
    int destination = (instruction >> 12) & 0xF;
    bool set_condition_codes = instruction & (1 << 20);
    bool is_imm = instruction & (1 << 25);
    int second_operand;
    stringstream second_operand_str;
    int shift = 0;

    if (is_imm)
    {
        second_operand = instruction & 0xFF;
        shift = (instruction & 0xF00) >> 7;

        second_operand = cpu.rotr32(second_operand, shift, false);
        second_operand_str << "#0x" << std::hex << second_operand;
    }
    else
    {
        second_operand = instruction & 0xF;
        second_operand_str << ARM_CPU::get_reg_name(second_operand);
        int shift_type = (instruction >> 5) & 0x3;

        switch (shift_type)
        {
            case 0:
                second_operand_str << ", lsl";
                break;
            case 1:
                second_operand_str << ", lsr";
                break;
            case 2:
                second_operand_str << ", asr";
                break;
            case 3:
                second_operand_str << ", ror";
                break;
        }

        if (instruction & (1 << 4))
        {
            second_operand_str << " " << ARM_CPU::get_reg_name((instruction >> 8) & 0xF);
        }
        else
        {
            second_operand_str << " #" << ((instruction >> 7) & 0x1F);
        }
    }

    switch (opcode)
    {
        case 0x0:
            output << "and";
            break;
        case 0x1:
            output << "eor";
            break;
        case 0x2:
            output << "sub";
            break;
        case 0x3:
            output << "rsb";
            break;
        case 0x4:
            output << "add";
            break;
        case 0x5:
            output << "adc";
            break;
        case 0x6:
            output << "sbc";
            break;
        case 0x7:
            output << "rsc";
            break;
        case 0x8:
            if (set_condition_codes)
                output << "tst";
            else
                return disasm_mrs(cpu, instruction);
            break;
        case 0x9:
            if (set_condition_codes)
                output << "teq";
            else
                return disasm_msr(cpu, instruction);
            break;
        case 0xA:
            if (set_condition_codes)
                output << "cmp";
            else
                return disasm_mrs(cpu, instruction);
            break;
        case 0xB:
            if (set_condition_codes)
                output << "cmn";
            else
                return disasm_msr(cpu, instruction);
            break;
        case 0xC:
            output << "orr";
            break;
        case 0xD:
            output << "mov";
            break;
        case 0xE:
            output << "bic";
            break;
        case 0xF:
            output << "mvn";
            break;
    }

    if (set_condition_codes)
        output << "s";

    output << ARM_CPU::get_condition_name(instruction >> 28);

    output << " " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(first_operand) << ", ";

    output << second_operand_str.str();
    return output.str();
}

string Disassembler::disasm_mrs(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    output << "mrs";
    output << ARM_CPU::get_condition_name(instruction >> 28) << " ";
    output << ARM_CPU::get_reg_name((instruction >> 12) & 0xF) << ", ";
    bool using_CPSR = (instruction & (1 << 22)) == 0;
    if (using_CPSR)
        output << "CPSR";
    else
        output << "SPSR";
    return output.str();
}

string Disassembler::disasm_msr(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    output << "msr";
    output << ARM_CPU::get_condition_name(instruction >> 28) << " ";

    bool using_CPSR = (instruction & (1 << 22)) == 0;
    if (using_CPSR)
        output << "CPSR";
    else
        output << "SPSR";

    output << ", ";

    bool is_imm = instruction & (1 << 25);
    if (is_imm)
    {
        int operand = instruction & 0xFF;
        int shift = (instruction & 0xFF0) >> 7;
        operand = cpu.rotr32(operand, shift, false);
        output << "#0x" << std::hex << operand;
    }
    else
        output << ARM_CPU::get_reg_name(instruction & 0xF);
    return output.str();
}

string Disassembler::disasm_load_store(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    if (instruction & (1 << 20))
        output << "ldr";
    else
        output << "str";
    if (instruction & (1 << 22))
        output << "b";
    output << ARM_CPU::get_condition_name(instruction >> 28);

    bool is_imm = (instruction & (1 << 25)) == 0;
    stringstream offset_str;
    if (is_imm)
    {
        offset_str << "#0x" << std::hex << (instruction & 0xFFF);
    }
    else
    {
        offset_str << "???";
    }

    bool pre_indexing = instruction & (1 << 24);
    int reg = (instruction >> 12) & 0xF;
    int base = (instruction >> 16) & 0xF;
    output << " " << ARM_CPU::get_reg_name(reg) << ", ";
    output << "[" << ARM_CPU::get_reg_name(base);

    if (pre_indexing)
        output << ", " << offset_str.str();
    output << "]";
    if (instruction & (1 << 21))
        output << "!";
    if (!pre_indexing)
        output << ", " << offset_str.str();
    return output.str();
}

string Disassembler::disasm_branch(ARM_CPU &cpu, uint32_t instruction, uint32_t address)
{
    stringstream output;
    int condition = instruction >> 28;
    int32_t offset = (instruction & 0xFFFFFF) << 2;
    offset <<= 6;
    offset >>= 6;
    if (condition == 15)
    {
        if (cpu.get_id())
            return "UNDEFINED";
        else
        {
            output << "blx";
            if (instruction & (1 << 24))
                offset += 2;
            output << " $0x" << std::hex << (address + offset);
            return output.str();
        }
    }

    output << "b";

    if (instruction & (1 << 24))
        output << "l";

    output << ARM_CPU::get_condition_name(condition);
    output << " $0x" << std::hex << (address + offset);

    return output.str();
}

string Disassembler::disasm_bx(uint32_t instruction)
{
    string output = "bx";
    output += ARM_CPU::get_condition_name(instruction >> 28);

    output += " ";
    output += ARM_CPU::get_reg_name(instruction & 0xF);
    return output;
}
