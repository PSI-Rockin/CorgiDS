/*
    CorgiDS Copyright PSISP 2017-2018
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
        case ARM_INSTR::COUNT_LEADING_ZEROS:
            output = disasm_count_leading_zeros(cpu, instruction);
            break;
        case ARM_INSTR::MULTIPLY:
            output = disasm_multiply(cpu, instruction);
            break;
        case ARM_INSTR::MULTIPLY_LONG:
            output = disasm_multiply_long(cpu, instruction);
            break;
        case ARM_INSTR::SIGNED_HALFWORD_MULTIPLY:
            output = disasm_signed_halfword_multiply(cpu, instruction);
            break;
        case ARM_INSTR::SWAP:
            output = disasm_swap(cpu, instruction);
            break;
        case ARM_INSTR::STORE_BYTE:
        case ARM_INSTR::LOAD_BYTE:
        case ARM_INSTR::STORE_WORD:
        case ARM_INSTR::LOAD_WORD:
            output = disasm_load_store(cpu, instruction, address);
            break;
        case ARM_INSTR::STORE_HALFWORD:
        case ARM_INSTR::LOAD_HALFWORD:
            output = disasm_load_store_halfword(cpu, instruction);
            break;
        case ARM_INSTR::LOAD_SIGNED_BYTE:
        case ARM_INSTR::LOAD_SIGNED_HALFWORD:
            output = disasm_load_signed(cpu, instruction);
            break;
        case ARM_INSTR::STORE_BLOCK:
        case ARM_INSTR::LOAD_BLOCK:
            output = disasm_load_store_block(cpu, instruction);
            break;
        case ARM_INSTR::BRANCH:
        case ARM_INSTR::BRANCH_WITH_LINK:
            output = disasm_branch(cpu, instruction, address);
            break;
        case ARM_INSTR::BRANCH_EXCHANGE:
        case ARM_INSTR::BRANCH_LINK_EXCHANGE:
            output = disasm_bx(cpu, instruction);
            break;
        case ARM_INSTR::COP_REG_TRANSFER:
            output = disasm_coprocessor_reg_transfer(cpu, instruction);
            break;
        default:
            return "UNDEFINED";
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
        stringstream shift_str;
        second_operand = instruction & 0xF;
        second_operand_str << ARM_CPU::get_reg_name(second_operand);
        int shift_type = (instruction >> 5) & 0x3;
        bool imm_shift = (instruction & (1 << 4)) == 0;
        shift = (instruction >> 7) & 0x1F;

        switch (shift_type)
        {
            case 0:
                if (!imm_shift || shift)
                    second_operand_str << ", lsl";
                break;
            case 1:
                second_operand_str << ", lsr";
                if (!shift)
                    shift = 32;
                break;
            case 2:
                second_operand_str << ", asr";
                if (!shift)
                    shift = 32;
                break;
            case 3:
                if (imm_shift && !shift)
                    second_operand_str << ", rrx";
                else
                    second_operand_str << ", ror";
                break;
        }

        if (!imm_shift)
        {
            second_operand_str << " " << ARM_CPU::get_reg_name((instruction >> 8) & 0xF);
        }
        else
        {
            if (shift || shift_type == 1 || shift_type == 2)
                second_operand_str << " #" << ((instruction >> 7) & 0x1F);
        }
    }

    bool use_first_op = true;
    bool use_dest = true;

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
            {
                output << "tst";
                use_dest = false;
                set_condition_codes = false;
            }
            else
                return disasm_mrs(cpu, instruction);
            break;
        case 0x9:
            if (set_condition_codes)
            {
                output << "teq";
                use_dest = false;
                set_condition_codes = false;
            }
            else
                return disasm_msr(cpu, instruction);
            break;
        case 0xA:
            if (set_condition_codes)
            {
                output << "cmp";
                use_dest = false;
                set_condition_codes = false;
            }
            else
                return disasm_mrs(cpu, instruction);
            break;
        case 0xB:
            if (set_condition_codes)
            {
                output << "cmn";
                use_dest = false;
                set_condition_codes = false;
            }
            else
                return disasm_msr(cpu, instruction);
            break;
        case 0xC:
            output << "orr";
            break;
        case 0xD:
            output << "mov";
            use_first_op = false;
            break;
        case 0xE:
            output << "bic";
            break;
        case 0xF:
            output << "mvn";
            use_first_op = false;
            break;
    }

    if (set_condition_codes)
        output << "s";

    output << ARM_CPU::get_condition_name(instruction >> 28) << " ";

    if (use_dest)
        output << ARM_CPU::get_reg_name(destination) << ", ";

    if (use_first_op)
        output << ARM_CPU::get_reg_name(first_operand) << ", ";

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

string Disassembler::disasm_count_leading_zeros(ARM_CPU &cpu, uint32_t instruction)
{
    if (cpu.get_id())
        return "UNDEFINED";

    stringstream output;
    uint32_t source = instruction & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;

    output << "clz " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(source);
    return output.str();
}

string Disassembler::disasm_multiply(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    bool accumulate = instruction & (1 << 21);
    bool set_condition_codes = instruction & (1 << 20);
    int destination = (instruction >> 16) & 0xF;
    int first_operand = instruction & 0xF;
    int second_operand = (instruction >> 8) & 0xF;
    int third_operand = (instruction >> 12) & 0xF;

    if (accumulate)
        output << "mla";
    else
        output << "mul";
    if (set_condition_codes)
        output << "s";

    output << ARM_CPU::get_condition_name(instruction >> 28);

    output << " " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(first_operand);
    output << ", " << ARM_CPU::get_reg_name(second_operand);
    if (accumulate)
        output << ", " << ARM_CPU::get_reg_name(third_operand);
    return output.str();
}

string Disassembler::disasm_multiply_long(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    bool is_signed = instruction & (1 << 22);
    bool accumulate = instruction & (1 << 21);
    bool set_condition_codes = instruction & (1 << 20);

    int dest_hi = (instruction >> 16) & 0xF;
    int dest_lo = (instruction >> 12) & 0xF;
    uint32_t first_operand = (instruction >> 8) & 0xF;
    uint32_t second_operand = (instruction) & 0xF;

    if (is_signed)
        output << "s";
    else
        output << "u";

    if (accumulate)
        output << "mlal";
    else
        output << "mull";
    if (set_condition_codes)
        output << "s";

    output << ARM_CPU::get_condition_name(instruction >> 28);
    output << " " << ARM_CPU::get_reg_name(dest_lo) << ", " << ARM_CPU::get_reg_name(dest_hi);
    output << ", " << ARM_CPU::get_reg_name(second_operand) << ", " << ARM_CPU::get_reg_name(first_operand);

    return output.str();
}

string Disassembler::disasm_signed_halfword_multiply(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    uint32_t destination = (instruction >> 16) & 0xF;
    uint32_t accumulate = (instruction >> 12) & 0xF;
    uint32_t first_operand = (instruction >> 8) & 0xF;
    uint32_t second_operand = instruction & 0xF;
    int opcode = (instruction >> 21) & 0xF;

    bool first_op_top = instruction & (1 << 6);
    bool second_op_top = instruction & (1 << 5);

    string first_op_str;
    string second_op_str;

    if (first_op_top)
        first_op_str = "t";
    else
        first_op_str = "b";

    if (second_op_top)
        second_op_str = "t";
    else
        second_op_str = "b";

    switch (opcode)
    {
        case 0xB:
            output << "smul" << first_op_str << second_op_str << ARM_CPU::get_condition_name(instruction >> 28);
            output << " " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(second_operand);
            output << ", " << ARM_CPU::get_reg_name(first_operand);
            break;
        default:
            return "UNDEFINED";
    }
    return output.str();
}

string Disassembler::disasm_swap(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    bool is_byte = instruction & (1 << 22);
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;
    uint32_t source = instruction & 0xF;

    output << "swp";
    if (is_byte)
        output << "b";

    output << ARM_CPU::get_condition_name(instruction >> 28);
    output << " " << ARM_CPU::get_reg_name(destination) << ", " << ARM_CPU::get_reg_name(source);
    output << ", [" << ARM_CPU::get_reg_name(base) << "]";
    return output.str();
}

string Disassembler::disasm_load_store(ARM_CPU &cpu, uint32_t instruction, uint32_t address)
{
    stringstream output;
    bool is_loading = instruction & (1 << 20);
    if (is_loading)
        output << "ldr";
    else
        output << "str";
    if (instruction & (1 << 22))
        output << "b";
    output << ARM_CPU::get_condition_name(instruction >> 28);

    bool is_adding_offset = instruction & (1 << 23);
    bool pre_indexing = instruction & (1 << 24);
    int reg = (instruction >> 12) & 0xF;
    int base = (instruction >> 16) & 0xF;
    bool is_imm = (instruction & (1 << 25)) == 0;
    stringstream offset_str;

    //Handle ldr =adr
    if (is_loading && base == REG_PC && is_imm)
    {
        uint32_t literal = address + 8;
        literal += instruction & 0xFFF;
        output << " " << ARM_CPU::get_reg_name(reg) << ", =0x";
        output << std::hex << cpu.read_word(literal);
        return output.str();
    }
    string sub_str;
    if (!is_adding_offset)
        sub_str = "-";
    if (is_imm)
    {
        int offset = instruction & 0xFFF;
        if (offset)
            offset_str << ", " << sub_str << "#0x" << std::hex << offset;
    }
    else
    {
        offset_str << ", " << sub_str << ARM_CPU::get_reg_name(instruction & 0xF);

        int shift_type = (instruction >> 5) & 0x3;
        int shift = (instruction >> 7) & 0x1F;

        switch (shift_type)
        {
            case 0:
                if (shift)
                    offset_str << ", lsl #" << shift;
                break;
            case 1:
                offset_str << ", lsr #";
                if (!shift)
                    shift = 32;
                offset_str << shift;
                break;
            case 2:
                offset_str << ", asr #";
                if (!shift)
                    shift = 32;
                offset_str << shift;
                break;
            case 3:
                if (!shift)
                    offset_str << ", rrx";
                else
                    offset_str << ", ror #" << shift;
                break;
        }
    }


    output << " " << ARM_CPU::get_reg_name(reg) << ", ";
    output << "[" << ARM_CPU::get_reg_name(base);

    if (pre_indexing)
        output << offset_str.str();
    output << "]";
    if (pre_indexing && (instruction & (1 << 21)))
        output << "!";
    if (!pre_indexing)
        output << offset_str.str();
    return output.str();
}

string Disassembler::disasm_load_store_halfword(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_imm_offset = (instruction & (1 << 22)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    bool is_loading = (instruction & (1 << 20)) != 0;
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t reg = (instruction >> 12) & 0xF;

    if (is_loading)
        output << "ldrh";
    else
        output << "strh";

    output << ARM_CPU::get_condition_name(instruction >> 28);

    output << " " << ARM_CPU::get_reg_name(reg) << ", [" << ARM_CPU::get_reg_name(base);

    int offset = ((instruction >> 4) & 0xF0) | (instruction & 0xF);
    stringstream offset_stream;
    string sub_str = "";
    if (!is_adding_offset)
        sub_str = "-";
    if (is_imm_offset)
    {
        if (offset)
            offset_stream << sub_str << "#" << std::hex << offset;
    }
    else
        offset_stream << sub_str << ARM_CPU::get_reg_name(offset & 0xF);

    string offset_str = offset_stream.str();
    if (is_preindexing)
    {
        if (offset_str.length())
            output << ", " << offset_str;
        output << "]";
        if (is_writing_back)
            output << "!";
    }
    else
    {
        output << "]";
        if (offset_str.length())
            output << ", " << offset_str;
    }

    return output.str();
}

string Disassembler::disasm_load_signed(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    bool is_preindexing = (instruction & (1 << 24)) != 0;
    bool is_adding_offset = (instruction & (1 << 23)) != 0;
    bool is_writing_back = (instruction & (1 << 21)) != 0;
    uint32_t base = (instruction >> 16) & 0xF;
    uint32_t destination = (instruction >> 12) & 0xF;

    bool is_imm_offset = (instruction & (1 << 22)) != 0;
    bool load_halfword = (instruction & (1 << 5)) != 0;

    output << "ldrs";
    if (load_halfword)
        output << "h";
    else
        output << "b";

    output << ARM_CPU::get_condition_name(instruction >> 28);
    output << " " << ARM_CPU::get_reg_name(destination) << ", [";
    output << ARM_CPU::get_reg_name(base);

    stringstream offset_str;
    string sub_str;
    if (!is_adding_offset)
        sub_str = "-";
    if (is_imm_offset)
    {
        int offset = ((instruction >> 4) & 0xF0) | (instruction & 0xF);
        if (offset)
            offset_str << sub_str << ", #0x" << std::hex << offset;
    }
    else
    {
        offset_str << sub_str << ", " << ARM_CPU::get_reg_name(instruction & 0xF);
    }
    if (is_preindexing)
        output << offset_str.str();
    output << "]";
    if (is_writing_back && is_preindexing)
        output << "!";
    if (!is_preindexing)
        output << offset_str.str();
    return output.str();
}

string Disassembler::disasm_load_store_block(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    uint16_t reg_list = instruction & 0xFFFF;
    uint32_t base = (instruction >> 16) & 0xF;
    bool load_block = instruction & (1 << 20);
    bool is_writing_back = instruction & (1 << 21);
    bool load_PSR = instruction & (1 << 22);
    bool is_adding_offset = instruction & (1 << 23);
    bool is_preindexing = instruction & (1 << 24);

    if (!load_block && base == REG_SP && !is_adding_offset && is_preindexing && is_writing_back)
        output << "push" << ARM_CPU::get_condition_name(instruction >> 28);
    else if (load_block && base == REG_SP && is_adding_offset && !is_preindexing && is_writing_back)
        output << "pop" << ARM_CPU::get_condition_name(instruction >> 28);
    else
    {
        if (load_block)
            output << "ldm";
        else
            output << "stm";
        if (is_adding_offset)
            output << "i";
        else
            output << "d";

        if (is_preindexing)
            output << "b";
        else
            output << "a";

        output << ARM_CPU::get_condition_name(instruction >> 28);

        output << " " << ARM_CPU::get_reg_name(base);
        if (is_writing_back)
            output << "!";

        output << ",";
    }

    output << " {";
    bool on_first_reg = true;
    int consecutive_regs = 0;

    for (int i = 0; i <= 16; i++)
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

    if (load_PSR)
        output << "^";
    return output.str();
}

string Disassembler::disasm_branch(ARM_CPU &cpu, uint32_t instruction, uint32_t address)
{
    stringstream output;
    int condition = instruction >> 28;
    int32_t offset = (instruction & 0xFFFFFF) << 2;
    offset <<= 6;
    offset >>= 6;
    offset += 8; //account for prefetch
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

string Disassembler::disasm_bx(ARM_CPU& cpu, uint32_t instruction)
{
    string output;
    if (instruction & (1 << 5))
    {
        if (cpu.get_id())
            return "UNDEFINED";
        else
            output = "blx";
    }
    else
        output = "bx";
    output += ARM_CPU::get_condition_name(instruction >> 28);

    output += " ";
    output += ARM_CPU::get_reg_name(instruction & 0xF);
    return output;
}

string Disassembler::disasm_coprocessor_reg_transfer(ARM_CPU &cpu, uint32_t instruction)
{
    stringstream output;
    int operation_mode = (instruction >> 21) & 0x7;
    bool is_loading = (instruction & (1 << 20)) != 0;
    uint32_t CP_reg = (instruction >> 16) & 0xF;
    uint32_t ARM_reg = (instruction >> 12) & 0xF;

    int coprocessor_id = (instruction >> 8) & 0xF;
    int coprocessor_info = (instruction >> 5) & 0x7;
    int coprocessor_operand = instruction & 0xF;

    if (is_loading)
        output << "mrc";
    else
        output << "mcr";

    output << " " << coprocessor_id << ", " << operation_mode << ", " << ARM_CPU::get_reg_name(ARM_reg);
    output << ", cr" << CP_reg << ", cr" << coprocessor_operand << ", {" << coprocessor_info << "}";
    return output.str();
}
