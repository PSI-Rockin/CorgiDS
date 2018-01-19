/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <sstream>
#include "disassembler.hpp"

using namespace std;

string Disassembler::disasm_thumb(ARM_CPU &cpu, uint16_t instruction, uint32_t address)
{
    THUMB_INSTR opcode = Interpreter::thumb_decode(instruction);

    string output = "";

    switch (opcode)
    {
        case THUMB_INSTR::MOV_SHIFT:
            output = disasm_thumb_mov_shift(cpu, instruction);
            break;
        case THUMB_INSTR::ADD_REG:
        case THUMB_INSTR::SUB_REG:
            output = disasm_thumb_op_reg(cpu, instruction);
            break;
        case THUMB_INSTR::ADD_IMM:
        case THUMB_INSTR::CMP_IMM:
        case THUMB_INSTR::MOV_IMM:
        case THUMB_INSTR::SUB_IMM:
            output = disasm_thumb_op_imm(cpu, instruction);
            break;
        case THUMB_INSTR::ALU_OP:
            output = disasm_thumb_alu_op(cpu, instruction);
            break;
        case THUMB_INSTR::HI_REG_OP:
            output = disasm_thumb_hi_reg_op(cpu, instruction);
            break;
        case THUMB_INSTR::PC_REL_LOAD:
            output = disasm_thumb_pc_rel_load(cpu, instruction);
            break;
        case THUMB_INSTR::STORE_REG_OFFSET:
        case THUMB_INSTR::LOAD_REG_OFFSET:
            output = disasm_thumb_load_store_reg(cpu, instruction);
            break;
        case THUMB_INSTR::LOAD_STORE_SIGN_HALFWORD:
            output = disasm_thumb_load_store_signed(cpu, instruction);
            break;
        case THUMB_INSTR::STORE_HALFWORD:
        case THUMB_INSTR::LOAD_HALFWORD:
            output = disasm_thumb_load_store_halfword(cpu, instruction);
            break;
        case THUMB_INSTR::STORE_IMM_OFFSET:
        case THUMB_INSTR::LOAD_IMM_OFFSET:
            output = disasm_thumb_load_store_imm(cpu, instruction);
            break;
        case THUMB_INSTR::SP_REL_STORE:
        case THUMB_INSTR::SP_REL_LOAD:
            output = disasm_thumb_sp_rel_load_store(cpu, instruction);
            break;
        case THUMB_INSTR::OFFSET_SP:
            output = disasm_thumb_offset_sp(cpu, instruction);
            break;
        case THUMB_INSTR::LOAD_ADDRESS:
            output = disasm_thumb_load_address(cpu, instruction);
            break;
        case THUMB_INSTR::POP:
        case THUMB_INSTR::PUSH:
            output = disasm_thumb_push_pop(cpu, instruction);
            break;
        case THUMB_INSTR::STORE_MULTIPLE:
        case THUMB_INSTR::LOAD_MULTIPLE:
            output = disasm_thumb_load_store_multiple(cpu, instruction);
            break;
        case THUMB_INSTR::BRANCH:
            output = disasm_thumb_branch(cpu, instruction);
            break;
        case THUMB_INSTR::COND_BRANCH:
            output = disasm_thumb_cond_branch(cpu, instruction);
            break;
        default:
            output = "UNDEFINED";
    }

    return output;
}

string Disassembler::disasm_thumb_mov_shift(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    int opcode = (instruction >> 11) & 0x3;
    int shift = (instruction >> 6) & 0x1F;
    uint32_t source = (instruction >> 3) & 0x7;
    uint32_t destination = instruction & 0x7;
    uint32_t value = cpu.get_register(source);

    switch (opcode)
    {
        case 0:
            if (shift)
                output << "lsls";
            else
                output << "movs";
            break;
        case 1:
            if (!shift)
                shift = 32;
            output << "lsrs";
            break;
        case 2:
            if (!shift)
                shift = 32;
            output << "asrs";
            break;
        default:
            return "UNDEFINED";
    }

    output << " " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(source);
    if (shift)
        output << ", #" << shift;

    return output.str();
}

string Disassembler::disasm_thumb_op_reg(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    uint32_t destination = instruction & 0x7;
    uint32_t source = (instruction >> 3) & 0x7;
    uint32_t operand = (instruction >> 6) & 0x7;
    bool sub = instruction & (1 << 9);
    bool is_imm = instruction & (1 << 10);

    if (!sub)
        output << "adds";
    else
        output << "subs";

    output << " " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(source) << ", ";
    if (is_imm)
        output << "#0x" << std::hex << operand;
    else
        output << ARM_CPU::get_reg_name(operand);
    return output.str();
}

string Disassembler::disasm_thumb_op_imm(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    const static string ops[] = {"movs", "cmp", "adds", "subs"};
    int opcode = (instruction >> 11) & 0x3;
    int dest = (instruction >> 8) & 0x7;
    int offset = instruction & 0xFF;

    output << ops[opcode] << " " << ARM_CPU::get_reg_name(dest) << ", #0x" << std::hex << offset;
    return output.str();
}

string Disassembler::disasm_thumb_alu_op(ARM_CPU &cpu, uint16_t instruction)
{
    const static string ops[] =
        {"ands", "eors", "lsls", "lsrs", "asrs", "adcs", "sbcs", "rors",
         "tst", "negs", "cmp", "cmn", "orrs", "muls", "bics", "mvns"};
    stringstream output;
    uint32_t destination = instruction & 0x7;
    uint32_t source = (instruction >> 3) & 0x7;
    int opcode = (instruction >> 6) & 0xF;

    output << ops[opcode] << " " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(source);
    return output.str();
}

string Disassembler::disasm_thumb_hi_reg_op(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    int opcode = (instruction >> 8) & 0x3;
    bool high_source = (instruction & (1 << 6)) != 0;
    bool high_dest = (instruction & (1 << 7)) != 0;

    uint32_t source = (instruction >> 3) & 0x7;
    source |= high_source * 8;
    uint32_t destination = instruction & 0x7;
    destination |= high_dest * 8;

    bool use_dest = true;

    switch (opcode)
    {
        case 0x0:
            output << "add";
            break;
        case 0x1:
            output << "cmp";
            break;
        case 0x2:
            output << "mov";
            break;
        case 0x3:
            if (high_dest)
                output << "blx";
            else
                output << "bx";
            use_dest = false;
            break;
    }

    output << " ";

    if (use_dest)
        output << ARM_CPU::get_reg_name(destination) << ", ";
    output << ARM_CPU::get_reg_name(source);
    return output.str();
}

string Disassembler::disasm_thumb_pc_rel_load(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    uint32_t destination = (instruction >> 8) & 0x7;
    uint32_t address = cpu.get_PC() + ((instruction & 0xFF) << 2);
    address &= ~0x3;
    output << "ldr " << ARM_CPU::get_reg_name(destination) << ", =0x" << std::hex << cpu.read_word(address);
    return output.str();
}

string Disassembler::disasm_thumb_load_store_reg(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    bool is_loading = instruction & (1 << 11);
    bool is_byte = instruction & (1 << 10);

    if (is_loading)
        output << "ldr";
    else
        output << "str";

    if (is_byte)
        output << "b";

    int offset = (instruction >> 6) & 0x7;
    int base = (instruction >> 3) & 0x7;
    int source_dest = instruction & 0x7;

    output << " " << ARM_CPU::get_reg_name(source_dest) << ", [" << ARM_CPU::get_reg_name(base);
    output << ", " << ARM_CPU::get_reg_name(offset) << "]";
    return output.str();
}

string Disassembler::disasm_thumb_load_store_signed(ARM_CPU &cpu, uint16_t instruction)
{
    const static string ops[] = {"strh", "ldsb", "ldrh", "ldsh"};
    stringstream output;
    uint32_t destination = instruction & 0x7;
    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t offset = (instruction >> 6) & 0x7;
    int opcode = (instruction >> 10) & 0x3;

    output << ops[opcode] << " " << ARM_CPU::get_reg_name(destination) << ", [" << ARM_CPU::get_reg_name(base);
    output << ", " << ARM_CPU::get_reg_name(offset) << "]";
    return output.str();
}

string Disassembler::disasm_thumb_load_store_halfword(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    uint32_t offset = ((instruction >> 6) & 0x1F) << 1;
    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t source_dest = instruction & 0x7;
    bool is_loading = instruction & (1 << 11);

    if (is_loading)
        output << "ldrh";
    else
        output << "strh";

    output << " " << ARM_CPU::get_reg_name(source_dest) << ", [" << ARM_CPU::get_reg_name(base);
    if (offset)
        output << ", #0x" << std::hex << offset;
    output << "]";

    return output.str();
}

string Disassembler::disasm_thumb_load_store_imm(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    bool is_loading = instruction & (1 << 11);
    bool is_byte = (instruction & (1 << 12));

    uint32_t base = (instruction >> 3) & 0x7;
    uint32_t source_dest = instruction & 0x7;
    uint32_t offset = (instruction >> 6) & 0x1F;

    if (is_loading)
        output << "ldr";
    else
        output << "str";

    if (is_byte)
        output << "b";
    else
        offset <<= 2;

    output << " " << ARM_CPU::get_reg_name(source_dest) << ", [" << ARM_CPU::get_reg_name(base);
    if (offset)
        output << ", #0x" << std::hex << offset;
    output << "]";

    return output.str();
}

string Disassembler::disasm_thumb_sp_rel_load_store(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    uint32_t source_dest = (instruction >> 8) & 0x7;
    uint32_t offset = (instruction & 0xFF) << 2;
    bool is_loading = instruction & (1 << 11);

    if (is_loading)
        output << "ldr";
    else
        output << "str";

    output << " " << ARM_CPU::get_reg_name(source_dest) << ", [sp";
    if (offset)
        output << ", #0x" << std::hex << offset;
    output << "]";
    return output.str();
}

string Disassembler::disasm_thumb_offset_sp(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    uint16_t offset = (instruction & 0x7F) << 2;

    if (instruction & (1 << 7))
        output << "sub";
    else
        output << "add";

    output << " sp, #0x" << offset;

    return output.str();
}

string Disassembler::disasm_thumb_load_address(ARM_CPU& cpu, uint16_t instruction)
{
    stringstream output;
    uint32_t destination = (instruction >> 8) & 0x7;
    uint32_t offset = (instruction & 0xFF) << 2;
    bool adding_SP = instruction & (1 << 11);

    output << "add " << ARM_CPU::get_reg_name(destination) << ", ";

    if (adding_SP)
        output << "sp, ";
    else
        output << "pc, ";

    output << "#0x" << std::hex << offset;

    return output.str();
}

string Disassembler::disasm_thumb_push_pop(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    bool is_loading = instruction & (1 << 11);
    bool use_LR_PC = instruction & (1 << 8);
    uint8_t reg_list = instruction & 0xFF;

    if (is_loading)
        output << "pop";
    else
        output << "push";

    output << " {";
    int consecutive_regs = 0;
    bool on_first_reg = true;

    //<= is used here so that consecutive regs ending at r7 will output correctly
    //e.g. push {r4-r7}
    //push {} would be outputted without <=
    for (int i = 0; i <= 8; i++)
    {
        if (reg_list & (1 << i))
            consecutive_regs++;
        else
        {
            if (consecutive_regs)
            {
                if (!on_first_reg)
                    output << ", ";
                output << ARM_CPU::get_reg_name(i - consecutive_regs);
                if (consecutive_regs > 1)
                    output << "-" << ARM_CPU::get_reg_name(i - 1);
                on_first_reg = false;
            }
            consecutive_regs = 0;
        }
    }
    if (use_LR_PC)
    {
        if (!on_first_reg)
            output << ", ";
        if (is_loading)
            output << "pc";
        else
            output << "lr";
    }
    output << "}";
    return output.str();
}

string Disassembler::disasm_thumb_load_store_multiple(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    uint8_t reg_list = instruction & 0xFF;
    uint32_t base = (instruction >> 8) & 0x7;
    bool is_loading = instruction & (1 << 11);

    if (is_loading)
        output << "ldmia";
    else
        output << "stmia";

    output << " " << ARM_CPU::get_reg_name(base) << "!, {";

    int consecutive_regs = 0;
    bool on_first_reg = true;
    for (int i = 0; i <= 8; i++)
    {
        if (reg_list & (1 << i))
            consecutive_regs++;
        else
        {
            if (consecutive_regs)
            {
                if (!on_first_reg)
                    output << ", ";
                output << ARM_CPU::get_reg_name(i - consecutive_regs);
                if (consecutive_regs > 1)
                    output << "-" << ARM_CPU::get_reg_name(i - 1);
                on_first_reg = false;
            }
            consecutive_regs = 0;
        }
    }

    output << "}";
    return output.str();
}

string Disassembler::disasm_thumb_branch(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    uint32_t address = cpu.get_PC();
    int16_t offset = (instruction & 0x7FF) << 1;

    //Sign extend 12-bit offset
    offset <<= 4;
    offset >>= 4;

    address += offset;

    output << "b $0x" << std::hex << address;
    return output.str();
}

string Disassembler::disasm_thumb_cond_branch(ARM_CPU &cpu, uint16_t instruction)
{
    stringstream output;
    int condition = (instruction >> 8) & 0xF;
    if (condition == 0xF)
    {
        output << "swi 0x" << std::hex << (instruction & 0xFF);
        return output.str();
    }

    uint32_t address = cpu.get_PC();
    int16_t offset = static_cast<int32_t>(instruction << 24) >> 23;

    output << "b" << ARM_CPU::get_condition_name(condition) << " $0x";
    output << std::hex << (address + offset);
    return output.str();
}

//Executed at the beginning of the 32-bit branch
string Disassembler::disasm_thumb_long_branch(ARM_CPU &cpu, uint32_t long_instr)
{
    stringstream output;
    bool switch_ARM = (long_instr & (1 << 28)) == 0;
    uint32_t address = cpu.get_PC();
    int32_t offset = (int32_t)((long_instr & 0x7FF) << 21) >> 9;
    address += offset;
    address += ((long_instr >> 16) & 0x7FF) << 1;

    if (switch_ARM)
    {
        output << "blx";
        address &= ~0x3;
    }
    else
        output << "bl";
    output << " $0x" << std::hex << address;
    return output.str();
}
