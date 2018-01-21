/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include "config.hpp"
#include "cpu.hpp"
#include "cpuinstrs.hpp"
#include "emulator.hpp"

using namespace std;

uint32_t PSR_Flags::get()
{
    uint32_t reg = 0;
    reg |= negative << 31;
    reg |= zero << 30;
    reg |= carry << 29;
    reg |= overflow << 28;
    reg |= sticky_overflow << 27;
    
    reg |= IRQ_disabled << 7;
    reg |= FIQ_disabled << 6;
    reg |= thumb_on << 5;
    
    reg |= static_cast<int>(mode);
    return reg;
}

void PSR_Flags::set(uint32_t value)
{
    negative = (value & (1 << 31)) != 0;
    zero = (value & (1 << 30)) != 0;
    carry = (value & (1 << 29)) != 0;
    overflow = (value & (1 << 28)) != 0;
    sticky_overflow = (value & (1 << 27)) != 0;
    
    IRQ_disabled = (value & (1 << 7)) != 0;
    FIQ_disabled = (value & (1 << 6)) != 0;
    thumb_on = (value & (1 << 5)) != 0;
    
    mode = static_cast<PSR_MODE>(value & 0x1F);
}

ARM_CPU::ARM_CPU(Emulator* e, int id) : e(e), cp15(nullptr), cpu_id(id)
{
    //Fill waitstates with dummy values to prevent bugs
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 4; j++)
            code_waitstates[i][j] = 1;
        for (int j = 0; j < 4; j++)
            code_waitstates[i][j] = 1;
    }
    //Initialize waitstates based upon GBATek's info
    //Note that 8-bit reads/writes have the same timings as 16 bit
    if (!id)
    {
        //ARM9 - timings here are doubled to represent 66 MHz
        //Of interest is that the ARM9 is incapable of sequential code fetches
        //Thus ALL code fetches are subject to the three cycle penalty... so much for 66 MHz
        code_waitstates[0x2][0] = 1;
        code_waitstates[0x3][0] = 8;
        code_waitstates[0x4][0] = 8;
        code_waitstates[0x5][0] = 10;
        code_waitstates[0x6][0] = 10;
        code_waitstates[0x7][0] = 8;
        code_waitstates[0xF][0] = 8;
        
        code_waitstates[0x2][1] = 1;
        code_waitstates[0x3][1] = 8;
        code_waitstates[0x4][1] = 8;
        code_waitstates[0x5][1] = 10;
        code_waitstates[0x6][1] = 10;
        code_waitstates[0x7][1] = 8;
        code_waitstates[0xF][1] = 8;
        
        code_waitstates[0x2][2] = 1;
        code_waitstates[0x3][2] = 4;
        code_waitstates[0x4][2] = 4;
        code_waitstates[0x5][2] = 5;
        code_waitstates[0x6][2] = 5;
        code_waitstates[0x7][2] = 4;
        code_waitstates[0xF][2] = 4;
        
        code_waitstates[0x2][3] = 1;
        code_waitstates[0x3][3] = 4;
        code_waitstates[0x4][3] = 4;
        code_waitstates[0x5][3] = 5;
        code_waitstates[0x6][3] = 5;
        code_waitstates[0x7][3] = 4;
        code_waitstates[0xF][3] = 4;
        
        //Nonsequential data fetches are also subject to a three cycle penalty
        //However the ARM9 *can* do sequential data fetches, allowing for some speedup
        data_waitstates[0x2][0] = 1;
        data_waitstates[0x3][0] = 2;
        data_waitstates[0x4][0] = 2;
        data_waitstates[0x5][0] = 4;
        data_waitstates[0x6][0] = 4;
        data_waitstates[0x7][0] = 2;
        data_waitstates[0xF][0] = 2;
        
        data_waitstates[0x2][1] = 1;
        data_waitstates[0x3][1] = 2;
        data_waitstates[0x4][1] = 2;
        data_waitstates[0x5][1] = 4;
        data_waitstates[0x6][1] = 4;
        data_waitstates[0x7][1] = 2;
        data_waitstates[0xF][1] = 2;
        
        data_waitstates[0x2][2] = 1;
        data_waitstates[0x3][2] = 2;
        data_waitstates[0x4][2] = 2;
        data_waitstates[0x5][2] = 2;
        data_waitstates[0x6][2] = 2;
        data_waitstates[0x7][2] = 2;
        data_waitstates[0xF][2] = 2;
        
        data_waitstates[0x2][3] = 1;
        data_waitstates[0x3][3] = 2;
        data_waitstates[0x4][3] = 2;
        data_waitstates[0x5][3] = 2;
        data_waitstates[0x6][3] = 2;
        data_waitstates[0x7][3] = 2;
        data_waitstates[0xF][3] = 2;
    }
    else
    {
        //ARM7 - not as much here as most waitstates only equal 1
        code_waitstates[0x2][0] = 2;
        code_waitstates[0x5][0] = 2;
        code_waitstates[0x6][0] = 2;
        
        code_waitstates[0x2][1] = 2;
        code_waitstates[0x5][1] = 2;
        code_waitstates[0x6][1] = 2;
        
        code_waitstates[0x2][2] = 1;
        
        data_waitstates[0x2][0] = 2;
        
        data_waitstates[0x2][1] = 2;
        
        data_waitstates[0x2][2] = 1;
    }
    if (cpu_id)
        exception_base = 0;
    else
        exception_base = 0xFFFF0000;
}

uint32_t ARM_CPU::read_word(uint32_t address)
{
    if (!cpu_id)
        return cp15->read_word(address);
    return e->arm7_read_word(address);
}

uint16_t ARM_CPU::read_halfword(uint32_t address)
{
    if (!cpu_id)
        return cp15->read_halfword(address);
    return e->arm7_read_halfword(address);
}

uint8_t ARM_CPU::read_byte(uint32_t address)
{
    if (!cpu_id)
        return cp15->read_byte(address);
    return e->arm7_read_byte(address);
}

void ARM_CPU::write_word(uint32_t address, uint32_t word)
{
    if (!cpu_id)
        cp15->write_word(address, word);
    else
        e->arm7_write_word(address, word);
}

void ARM_CPU::write_halfword(uint32_t address, uint16_t halfword)
{
    if (!cpu_id)
        cp15->write_halfword(address, halfword);
    else
        e->arm7_write_halfword(address, halfword);
}

void ARM_CPU::write_byte(uint32_t address, uint8_t byte)
{
    if (!cpu_id)
        cp15->write_byte(address, byte);
    else
        e->arm7_write_byte(address, byte);
}

void ARM_CPU::power_on()
{
    halted = false;
    timestamp = 0;
    CPSR.thumb_on = false;
    CPSR.mode = PSR_MODE::SUPERVISOR;
    
    jp(exception_base, true); //Jump to reset vector
}

void ARM_CPU::direct_boot(uint32_t entry_point)
{
    jp(entry_point, true);
    regs[12] = entry_point;
    regs[13] = entry_point;
    CPSR.mode = PSR_MODE::SYSTEM;
    
    if (!cpu_id)
    {
        regs[13] = 0x030027FC;
        SP_irq = 0x03003F80;
        SP_svc = 0x03003FC0;
    }
    else
    {
        regs[13] = 0x0380FD80;
        SP_irq = 0x0380FF80;
        SP_svc = 0x0380FFC0;
    }
}

void ARM_CPU::gba_boot(bool direct)
{
    if (direct)
        jp(GBA_ROM_START, true);
    else
        jp(0, true);
    regs[13] = 0x03007F00;
    SP_svc = 0x03007FE0;
    SP_irq = 0x03007FA0;
    CPSR.mode = PSR_MODE::SYSTEM;
}

void ARM_CPU::set_cp15(CP15 *cp)
{
    cp15 = cp;
    cp15->link_with_cpu(this);
}

void ARM_CPU::execute()
{
    last_timestamp = timestamp;
    //TODO: replace these comparisons with a generic "halt state" variable
    if (halted)
    {
        timestamp += 4;
        if (e->requesting_interrupt(cpu_id))
        {
            halted = false;
            if (!CPSR.IRQ_disabled)
                handle_IRQ();
        }
        return;
    }

    //TODO: replace if-else with a function pointer depending on thumb state?
    if (!CPSR.thumb_on)
    {
        current_instr = read_word(regs[15] - 4);
        //if (Config::test)
            //e->mark_as_arm(regs[15] - 4);
        add_s32_code(regs[15] - 4, 1);
        regs[15] += 4;
        Interpreter::arm_interpret(*this);
    }
    else
    {
        current_instr = read_halfword(regs[15] - 2);
        //if (Config::test)
            //e->mark_as_thumb(regs[15] - 4);
        add_s16_code(regs[15] - 2, 1);
        regs[15] += 2;
        Interpreter::thumb_interpret(*this);
    }
    if (e->requesting_interrupt(cpu_id) && !CPSR.IRQ_disabled)
        handle_IRQ();
}

void ARM_CPU::jp(uint32_t new_addr, bool change_thumb_state)
{
    /*if (new_addr == 0x02013218)
        Config::test = true;
    if (new_addr == 0x02006124)
        Config::test = false;*/
    regs[15] = new_addr;
    
    //Simulate pipeline clear by adding extra cycles
    add_n32_code(new_addr, 1);
    add_s32_code(new_addr, 1);
    
    if (change_thumb_state)
        CPSR.thumb_on = new_addr & 0x1;
    
    regs[15] &= (CPSR.thumb_on) ? ~0x1 : ~0x3; //alignment safety
    
    //Prefetch
    regs[15] += (CPSR.thumb_on) ? 2 : 4;
}

void ARM_CPU::handle_UNDEFINED()
{
    throw "[CPU] Undefined instruction";
    /*printf("\nUNDEFINED bullshit!");
    uint32_t value = CPSR.get();
    SPSR[static_cast<int>(PSR_MODE::UNDEFINED)].set(value);

    LR_und = regs[15] - 4;
    update_reg_mode(PSR_MODE::UNDEFINED);
    CPSR.mode = PSR_MODE::UNDEFINED;
    CPSR.IRQ_disabled = true;
    jp(exception_base + 0x04, true);*/
}

void ARM_CPU::handle_IRQ()
{
    uint32_t value = CPSR.get();
    SPSR[static_cast<int>(PSR_MODE::IRQ)].set(value);
    
    //Update new CPSR
    LR_irq = regs[15] + ((CPSR.thumb_on) ? 2 : 0);
    update_reg_mode(PSR_MODE::IRQ);
    CPSR.mode = PSR_MODE::IRQ;
    CPSR.IRQ_disabled = true;
    jp(exception_base + 0x18, true);
}

void ARM_CPU::update_reg_mode(PSR_MODE new_mode)
{
    if (new_mode != CPSR.mode)
    {
        switch (CPSR.mode)
        {
            case PSR_MODE::SYSTEM:
            case PSR_MODE::USER:
                break;
            case PSR_MODE::IRQ:
                swap(regs[13], SP_irq);
                swap(regs[14], LR_irq);
                break;
            case PSR_MODE::FIQ:
                swap(regs[8], fiq_regs[0]);
                swap(regs[9], fiq_regs[1]);
                swap(regs[10], fiq_regs[2]);
                swap(regs[11], fiq_regs[3]);
                swap(regs[12], fiq_regs[4]);
                swap(regs[13], SP_fiq);
                swap(regs[14], LR_fiq);
                break;
            case PSR_MODE::SUPERVISOR:
                swap(regs[13], SP_svc);
                swap(regs[14], LR_svc);
                break;
            case PSR_MODE::UNDEFINED:
                swap(regs[13], SP_und);
                swap(regs[14], LR_und);
                break;
            default:
                throw "Unrecognized PSR mode";
        }

        switch (new_mode)
        {
            case PSR_MODE::SYSTEM:
            case PSR_MODE::USER:
                break;
            case PSR_MODE::IRQ:
                swap(regs[13], SP_irq);
                swap(regs[14], LR_irq);
                break;
            case PSR_MODE::FIQ:
                swap(regs[8], fiq_regs[0]);
                swap(regs[9], fiq_regs[1]);
                swap(regs[10], fiq_regs[2]);
                swap(regs[11], fiq_regs[3]);
                swap(regs[12], fiq_regs[4]);
                swap(regs[13], SP_fiq);
                swap(regs[14], LR_fiq);
                break;
            case PSR_MODE::SUPERVISOR:
                swap(regs[13], SP_svc);
                swap(regs[14], LR_svc);
                break;
            case PSR_MODE::UNDEFINED:
                swap(regs[13], SP_und);
                swap(regs[14], LR_und);
                break;
            default:
                throw "Unrecognized PSR mode";
        }
    }
}

void ARM_CPU::halt()
{
    halted = true;
}

void ARM_CPU::handle_SWI()
{
    if (Config::hle_bios && e->hle_bios(cpu_id))
        return;
    uint32_t value = CPSR.get();
    SPSR[static_cast<int>(PSR_MODE::SUPERVISOR)].set(value);
    
    uint32_t LR = regs[15];
    if (CPSR.thumb_on)
        LR -= 2;
    else
        LR -= 4;
    
    LR_svc = LR;
    update_reg_mode(PSR_MODE::SUPERVISOR);
    CPSR.mode = PSR_MODE::SUPERVISOR;
    CPSR.IRQ_disabled = true;
    jp(exception_base + 0x08, true);
}

void ARM_CPU::print_info()
{
    printf("\n");
    for (unsigned int i = 0; i < 16; i++)
    {
        uint32_t reg = get_register(i);
        
        switch (i)
        {
            case 10:
                printf("sl");
                break;
            case 11:
                printf("fp");
                break;
            case 12:
                printf("ip");
                break;
            case 13:
                printf("sp");
                break;
            case 14:
                printf("lr");
                break;
            case 15:
                printf("pc");
                break;
            default:
                printf("r%d", i);
                break;
        }
        
        printf(": $%08X", reg);
        if (i % 4 == 3)
            printf("\n");
        else
            printf("\t");
    }
    printf("CPSR Flags: ");
    (CPSR.negative) ? printf("N") : printf("-");
    (CPSR.zero) ? printf("Z") : printf("-");
    (CPSR.carry) ? printf("C") : printf("-");
    (CPSR.overflow) ? printf("V") : printf("-");
    (CPSR.sticky_overflow) ? printf("Q") : printf("-");
    
    printf("\tMode: ");
    switch (CPSR.mode)
    {
        case PSR_MODE::SYSTEM:
            printf("sys");
            break;
        case PSR_MODE::IRQ:
            printf("irq");
            break;
        case PSR_MODE::SUPERVISOR:
            printf("svc");
            break;
        default:
            printf("???");
            break;
    }
}

std::string ARM_CPU::get_condition_name(int id)
{
    switch (id)
    {
        case 0:
            return "eq";
        case 1:
            return "ne";
        case 2:
            return "cs";
        case 3:
            return "cc";
        case 4:
            return "mi";
        case 5:
            return "pl";
        case 6:
            return "vs";
        case 7:
            return "vc";
        case 8:
            return "hi";
        case 9:
            return "ls";
        case 10:
            return "ge";
        case 11:
            return "lt";
        case 12:
            return "gt";
        case 13:
            return "le";
        case 14:
            return "";
        default:
            return "??";
    }
}

std::string ARM_CPU::get_reg_name(int id)
{
    if (id < 10)
    {
        std::stringstream output;
        output << "r" << id;
        return output.str();
    }
    else
    {
        switch (id)
        {
            case 10:
                return "sl";
            case 11:
                return "fp";
            case 12:
                return "ip";
            case 13:
                return "sp";
            case 14:
                return "lr";
            case 15:
                return "pc";
        }
    }
    return "??";
}

int ARM_CPU::get_id()
{
    return cpu_id;
}

CP15* ARM_CPU::get_cp15()
{
    return cp15;
}

PSR_Flags* ARM_CPU::get_CPSR()
{
    return &CPSR;
}

void ARM_CPU::print_condition(int condition)
{
    switch (condition)
    {
        case 0x0:
            printf("EQ");
            break;
        case 0x1:
            printf("NE");
            break;
        case 0x2:
            printf("CS");
            break;
        case 0x3:
            printf("CC");
            break;
        case 0x4:
            printf("MI");
            break;
        case 0x5:
            printf("PL");
            break;
        case 0x6:
            printf("VS");
            break;
        case 0x7:
            printf("VC");
            break;
        case 0x8:
            printf("HI");
            break;
        case 0x9:
            printf("LS");
            break;
        case 0xA:
            printf("GE");
            break;
        case 0xB:
            printf("LT");
            break;
        case 0xC:
            printf("GT");
            break;
        case 0xD:
            printf("LE");
            break;
    }
}

bool ARM_CPU::check_condition(int condition)
{
    switch (condition)
    {
        case 0x0:
            //EQ - equal
            return CPSR.zero;
        case 0x1:
            //NE - not equal
            return !CPSR.zero;
        case 0x2:
            //CS - unsigned higher or same
            return CPSR.carry;
        case 0x3:
            //CC - unsigned lower
            return !CPSR.carry;
        case 0x4:
            //MI - negative
            return CPSR.negative;
        case 0x5:
            //PL - positive or zero
            return !CPSR.negative;
        case 0x6:
            //VS - overflow
            return CPSR.overflow;
        case 0x7:
            //VC - no overflow
            return !CPSR.overflow;
        case 0x8:
            //HI - unsigned higher
            return CPSR.carry && !CPSR.zero;
        case 0x9:
            //LS - unsigned lower or same
            return !CPSR.carry || CPSR.zero;
        case 0xA:
            //GE - greater than or equal
            return (CPSR.negative == CPSR.overflow);
        case 0xB:
            //LT - less than
            return (CPSR.negative != CPSR.overflow);
        case 0xC:
            //GT - greater than
            return !CPSR.zero && (CPSR.negative == CPSR.overflow);
        case 0xD:
            //LE - less than or equal to
            return CPSR.zero || (CPSR.negative != CPSR.overflow);
        case 0xE:
            //AL - always
            return true;
        case 0xF:
            //Not supposed to happen - ignore if it does
            return false;
        default:
            printf("\nUnrecognized condition %d", condition);
            throw "[CPU] Unrecognized condition code";
    }
}

void ARM_CPU::add_n32_code(uint32_t address, int cycles)
{
    timestamp += (1 + code_waitstates[(address & 0x0F000000) >> 24][0]) * cycles;
}

void ARM_CPU::add_s32_code(uint32_t address, int cycles)
{
    timestamp += (1 + code_waitstates[(address & 0x0F000000) >> 24][1]) * cycles;
}

void ARM_CPU::add_n16_code(uint32_t address, int cycles)
{
    timestamp += (1 + code_waitstates[(address & 0x0F000000) >> 24][2]) * cycles;
}

void ARM_CPU::add_s16_code(uint32_t address, int cycles)
{
    timestamp += (1 + code_waitstates[(address & 0x0F000000) >> 24][3]) * cycles;
}

void ARM_CPU::add_n32_data(uint32_t address, int cycles)
{
    timestamp += (1 + data_waitstates[(address & 0x0F000000) >> 24][0]) * cycles;
}

void ARM_CPU::add_s32_data(uint32_t address, int cycles)
{
    timestamp += (1 + data_waitstates[(address & 0x0F000000) >> 24][1]) * cycles;
}

void ARM_CPU::add_n16_data(uint32_t address, int cycles)
{
    timestamp += (1 + data_waitstates[(address & 0x0F000000) >> 24][2]) * cycles;
}

void ARM_CPU::add_s16_data(uint32_t address, int cycles)
{
    timestamp += (1 + data_waitstates[(address & 0x0F000000) >> 24][3]) * cycles;
}

void ARM_CPU::update_code_waitstate(uint8_t region, int n32_cycles, int s32_cycles, int n16_cycles, int s16_cycles)
{
    code_waitstates[region][0] = n32_cycles;
    code_waitstates[region][1] = s32_cycles;
    code_waitstates[region][2] = n16_cycles;
    code_waitstates[region][3] = s16_cycles;
}

void ARM_CPU::update_data_waitstate(uint8_t region, int n32_cycles, int s32_cycles, int n16_cycles, int s16_cycles)
{
    data_waitstates[region][0] = n32_cycles;
    data_waitstates[region][1] = s32_cycles;
    data_waitstates[region][2] = n16_cycles;
    data_waitstates[region][3] = s16_cycles;
}

void ARM_CPU::andd(int destination, int source, int operand, bool set_condition_codes)
{
    uint32_t result = source & operand;
    set_register(destination, result);
    
    if (set_condition_codes)
        set_zero_neg_flags(result);
}

void ARM_CPU::orr(int destination, int source, int operand, bool set_condition_codes)
{
    uint32_t result = source | operand;
    set_register(destination, result);
    
    if (set_condition_codes)
        set_zero_neg_flags(result);
}

void ARM_CPU::eor(int destination, int source, int operand, bool set_condition_codes)
{
    uint32_t result = source ^ operand;
    set_register(destination, result);
    
    if (set_condition_codes)
        set_zero_neg_flags(result);
}

void ARM_CPU::add(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes)
{
    uint64_t unsigned_result = source + operand;
    if (destination == REG_PC)
    {
        if (set_condition_codes)
            throw "[CPU] Op 'adds pc' unsupported";
        else
            jp(unsigned_result & 0xFFFFFFFF, true);
    }
    else
    {
        set_register(destination, unsigned_result & 0xFFFFFFFF);
        
        if (set_condition_codes)
            cmn(source, operand);
    }
}

void ARM_CPU::sub(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes)
{
    uint64_t unsigned_result = source - operand;
    if (destination == REG_PC)
    {
        if (set_condition_codes)
        {
            int index = static_cast<int>(CPSR.mode);
            update_reg_mode(SPSR[index].mode);
            CPSR.set(SPSR[index].get());
            jp(unsigned_result & 0xFFFFFFFF, false);
        }
        else
            jp(unsigned_result & 0xFFFFFFFF, true);
    }
    else
    {
        set_register(destination, unsigned_result & 0xFFFFFFFF);
        if (set_condition_codes)
            cmp(source, operand);
    }
}

void ARM_CPU::adc(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes)
{
    uint8_t carry = (CPSR.carry) ? 1 : 0;
    add(destination, source + carry, operand, set_condition_codes);
    if (set_condition_codes)
    {
        uint32_t temp = source + operand;
        uint32_t res = temp + carry;
        CPSR.carry = CARRY_ADD(source, operand) | CARRY_ADD(temp, carry);
        CPSR.overflow = ADD_OVERFLOW(source, operand, temp) | ADD_OVERFLOW(temp, carry, res);
    }
}

void ARM_CPU::sbc(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes)
{
    int borrow = (CPSR.carry) ? 0 : 1;
    sub(destination, source, operand + borrow, set_condition_codes);
    if (set_condition_codes)
    {
        uint32_t temp = source - operand;
        uint32_t res = temp - borrow;
        CPSR.carry = CARRY_SUB(source, operand) & CARRY_SUB(temp, borrow);
        CPSR.overflow = SUB_OVERFLOW(source, operand, temp) | SUB_OVERFLOW(temp, borrow, res);
    }
}

void ARM_CPU::tst(uint32_t x, uint32_t y)
{
    set_zero_neg_flags(x & y);
}

void ARM_CPU::teq(uint32_t x, uint32_t y)
{
    set_zero_neg_flags(x ^ y);
}

void ARM_CPU::cmn(uint32_t x, uint32_t y)
{
    uint32_t result = x + y;
    set_zero_neg_flags(result);
    set_CV_add_flags(x, y, result);
}

void ARM_CPU::cmp(uint32_t x, uint32_t y)
{
    uint32_t result = x - y;
    set_zero_neg_flags(result);
    set_CV_sub_flags(x, y, result);
}

void ARM_CPU::mov(uint32_t destination, uint32_t operand, bool alter_flags)
{
    if (destination == REG_PC)
    {
        if (alter_flags)
        {
            int index = static_cast<int>(CPSR.mode);
            update_reg_mode(SPSR[index].mode);
            CPSR.set(SPSR[index].get());
            jp(operand, false);
        }
        else
            jp(operand, true);
    }
    else
    {
        set_register(destination, operand);
        if (alter_flags)
            set_zero_neg_flags(operand);
    }
}

void ARM_CPU::mul(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes)
{
    uint64_t result = source * operand;
    uint32_t truncated = result & 0xFFFFFFFF;
    set_register(destination, truncated);
    
    if (set_condition_codes)
        set_zero_neg_flags(truncated);
}

void ARM_CPU::bic(uint32_t destination, uint32_t source, uint32_t operand, bool alter_flags)
{
    uint32_t result = source & ~operand;
    set_register(destination, result);
    if (alter_flags)
        set_zero_neg_flags(result);
}

void ARM_CPU::mvn(uint32_t destination, uint32_t operand, bool alter_flags)
{
    set_register(destination, ~operand);
    if (alter_flags)
        set_zero_neg_flags(~operand);
}

void ARM_CPU::mrs(uint32_t instruction)
{
    bool using_CPSR = (instruction & (1 << 22)) == 0;
    uint32_t destination = (instruction >> 12) & 0xF;
    
    if (using_CPSR)
    {
        set_register(destination, CPSR.get());
    }
    else
    {
        set_register(destination, SPSR[static_cast<int>(CPSR.mode)].get());
    }
}

void ARM_CPU::msr(uint32_t instruction)
{
    bool is_imm = (instruction & (1 << 25));
    bool using_CPSR = (instruction & (1 << 22)) == 0;

    PSR_Flags* PSR;
    if (using_CPSR)
        PSR = &CPSR;
    else
        PSR = &SPSR[static_cast<int>(CPSR.mode)];

    uint32_t value = PSR->get();
    uint32_t source;
    if (is_imm)
    {
        source = instruction & 0xFF;
        int shift = (instruction & 0xF00) >> 7;
        source = rotr32(source, shift, false);
    }
    else
        source = get_register(instruction & 0xF);

    uint32_t bitmask = 0;
    if (instruction & (1 << 19))
        bitmask |= 0xFF000000;

    if (instruction & (1 << 16))
        bitmask |= 0xFF;

    if (CPSR.mode == PSR_MODE::USER)
        bitmask &= 0xFFFFFF00; //prevent changes to control field

    if (using_CPSR)
        bitmask &= 0xFFFFFFDF; //prevent changes to thumb state

    value &= ~bitmask;
    value |= (source & bitmask);

    if (using_CPSR)
    {
        PSR_MODE new_mode = static_cast<PSR_MODE>(value & 0x1F);
        update_reg_mode(new_mode);
    }
    PSR->set(value);
}

void ARM_CPU::set_zero(bool cond)
{
    CPSR.zero = cond;
}

void ARM_CPU::set_neg(bool cond)
{
    CPSR.negative = cond;
}

//Code here from melonDS's ALU core (my original implementation was incorrect in many ways)
void ARM_CPU::set_CV_add_flags(uint32_t a, uint32_t b, uint32_t result)
{
    CPSR.carry = ((0xFFFFFFFF-a) < b);
    CPSR.overflow = ADD_OVERFLOW(a, b, result);
}

void ARM_CPU::set_CV_sub_flags(uint32_t a, uint32_t b, uint32_t result)
{
    CPSR.carry = a >= b;
    CPSR.overflow = SUB_OVERFLOW(a, b, result);
}

void ARM_CPU::spsr_to_cpsr()
{
    uint32_t new_CPSR = SPSR[static_cast<int>(CPSR.mode)].get();
    update_reg_mode(static_cast<PSR_MODE>(new_CPSR & 0x1F));
    CPSR.set(new_CPSR);
}

uint32_t ARM_CPU::lsl(uint32_t value, int shift, bool alter_flags)
{
    if (!shift)
    {
        if (alter_flags)
            set_zero_neg_flags(value);
        return value;
    }
    if (shift > 31)
    {
        if (alter_flags)
        {
            set_zero_neg_flags(0);
            CPSR.carry = value & (1 << 0);
        }
        return 0;
    }
    uint32_t result = value << shift;
    if (alter_flags)
    {
        set_zero_neg_flags(result);
        CPSR.carry = value & (1 << (32 - shift));
    }
    return value << shift;
}

uint32_t ARM_CPU::lsr(uint32_t value, int shift, bool alter_flags)
{
    if (shift > 31)
        return lsr_32(value, alter_flags);
    uint32_t result = value >> shift;
    if (alter_flags)
    {
        set_zero_neg_flags(result);
        if (shift)
            CPSR.carry = value & (1 << (shift - 1));
    }
    return result;
}

uint32_t ARM_CPU::lsr_32(uint32_t value, bool alter_flags)
{
    if (alter_flags)
    {
        set_zero_neg_flags(0);
        CPSR.carry = value & (1 << 31);
    }
    return 0;
}

uint32_t ARM_CPU::asr(uint32_t value, int shift, bool alter_flags)
{
    if (shift > 31)
        return asr_32(value, alter_flags);
    uint32_t result = static_cast<int32_t>(value) >> shift;
    if (alter_flags)
    {
        set_zero_neg_flags(result);
        if (shift)
            CPSR.carry = value & (1 << (shift - 1));
    }
    return result;
}

uint32_t ARM_CPU::asr_32(uint32_t value, bool alter_flags)
{
    uint32_t result = static_cast<int32_t>(value) >> 31;
    if (alter_flags)
    {
        set_zero_neg_flags(result);
        CPSR.carry = value & (1 << 31);
    }
    return result;
}

uint32_t ARM_CPU::rrx(uint32_t value, bool alter_flags)
{
    uint32_t result = value;
    result >>= 1;
    result |= (CPSR.carry) ? (1 << 31) : 0;
    if (alter_flags)
    {
        set_zero_neg_flags(result);
        CPSR.carry = value & 0x1;
    }
    return result;
}

//Thanks to Stack Overflow for providing a safe rotate function
uint32_t ARM_CPU::rotr32 (uint32_t n, unsigned int c, bool alter_flags)
{
    const unsigned int mask = 0x1F;
    if (alter_flags && c)
        CPSR.carry = n & (1 << (c - 1));
    c &= mask;

    uint32_t result = (n>>c) | (n<<( (-c)&mask ));

    if (alter_flags)
    {
        set_zero_neg_flags(result);
    }

    return result;
}
