/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef cpu_hpp
#define cpu_hpp
#include <cstdint>
#include <assert.h>
#include <limits.h>
#include <string>
#include "cp15.hpp"

#define REG_SP 13
#define REG_LR 14
#define REG_PC 15

//Carry/overflow macros from melonDS <3
//I hate ARM
#define CARRY_ADD(a, b)  ((0xFFFFFFFF-a) < b)
#define CARRY_SUB(a, b)  (a >= b)

#define ADD_OVERFLOW(a, b, result) ((!(((a) ^ (b)) & 0x80000000)) && (((a) ^ (result)) & 0x80000000))
#define SUB_OVERFLOW(a, b, result) (((a) ^ (b)) & 0x80000000) && (((a) ^ (result)) & 0x80000000)

enum class PSR_MODE
{
    USER = 0x10,
    FIQ = 0x11,
    IRQ = 0x12,
    SUPERVISOR = 0x13,
    ABORT = 0x17,
    UNDEFINED = 0x1B,
    SYSTEM = 0x1F
};

struct PSR_Flags
{
    bool negative;
    bool zero;
    bool carry;
    bool overflow;
    bool sticky_overflow;
    
    bool IRQ_disabled;
    bool FIQ_disabled;
    bool thumb_on;
    
    PSR_MODE mode;
    
    uint32_t get();
    void set(uint32_t value);
};

class Emulator;

class ARM_CPU
{
    private:
        Emulator* e;
        CP15* cp15;
        int cpu_id;
        bool halted;
    
        uint32_t SP_svc, SP_irq, SP_fiq, SP_abt, SP_und;
        uint32_t LR_svc, LR_irq, LR_fiq, LR_abt, LR_und;
        uint32_t fiq_regs[5];

        uint32_t regs[16];
    
        PSR_Flags CPSR;
        PSR_Flags SPSR[0x20];
    
        uint32_t exception_base;
    
        uint64_t timestamp;
        uint64_t last_timestamp;
        uint32_t current_instr;
    
        //Waitstates are as follows: n32, s32, n16, s16
        //Organized by the second-highest nibble in the address
        //e.g. an n32 mem read of 0x02000000 is waitstate[2][0]
        //TODO: waitstate for cache misses and GBA ROM reads?
        int code_waitstates[16][4];
        int data_waitstates[16][4];
    public:
        ARM_CPU(Emulator* e, int id);
        void set_cp15(CP15* cp);
        void power_on();
        void direct_boot(uint32_t entry_point);
        void gba_boot(bool direct);
        void run();
        void execute();
        void jp(uint32_t new_addr, bool change_thumb_state);
        void handle_UNDEFINED();
        void handle_IRQ();
        void halt();
        void handle_SWI();
        void print_info();
        void update_reg_mode(PSR_MODE new_mode);

        static std::string get_reg_name(int id);
        static std::string get_condition_name(int id);
    
        int get_id();
        CP15* get_cp15();
        uint32_t get_register(int id);
        uint64_t get_timestamp();
        int64_t cycles_ran();
        uint32_t get_PC();
        uint32_t get_current_instr();
        PSR_Flags* get_CPSR();
    
        void set_register(int id, uint32_t value);
    
        void print_condition(int condition);
        bool check_condition(int condition);
    
        uint32_t read_word(uint32_t address);
        uint16_t read_halfword(uint32_t address);
        uint8_t read_byte(uint32_t address);
    
        void write_word(uint32_t address, uint32_t word);
        void write_halfword(uint32_t address, uint16_t halfword);
        void write_byte(uint32_t address, uint8_t byte);
    
        //Waitstate bullshit
        void add_n32_code(uint32_t address, int cycles);
        void add_s32_code(uint32_t address, int cycles);
        void add_n16_code(uint32_t address, int cycles);
        void add_s16_code(uint32_t address, int cycles);
        void add_n32_data(uint32_t address, int cycles);
        void add_s32_data(uint32_t address, int cycles);
        void add_n16_data(uint32_t address, int cycles);
        void add_s16_data(uint32_t address, int cycles);

        void update_code_waitstate(uint8_t region, int n32_cycles, int s32_cycles, int n16_cycles, int s16_cycles);
        void update_data_waitstate(uint8_t region, int n32_cycles, int s32_cycles, int n16_cycles, int s16_cycles);
    
        void add_internal_cycles(int cycles);
        void add_cop_cycles(int cycles);
    
        //All data manipulation methods here
        void andd(int destination, int source, int operand, bool set_condition_codes);
        void orr(int destination, int source, int operand, bool set_condition_codes);
        void eor(int destination, int source, int operand, bool set_condition_codes);
        void add(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes);
        void sub(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes);
        void adc(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes);
        void sbc(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes);
        void cmp(uint32_t x, uint32_t y);
        void cmn(uint32_t x, uint32_t y);
        void tst(uint32_t x, uint32_t y);
        void teq(uint32_t x, uint32_t y);
        void mov(uint32_t destination, uint32_t operand, bool alter_flags);
        void mul(uint32_t destination, uint32_t source, uint32_t operand, bool set_condition_codes);
        void bic(uint32_t destination, uint32_t source, uint32_t operand, bool alter_flags);
        void mvn(uint32_t destination, uint32_t operand, bool alter_flags);
        void mrs(uint32_t instruction);
        void msr(uint32_t instruction);
    
        void set_zero(bool cond);
        void set_neg(bool cond);
        void set_zero_neg_flags(uint32_t value);
        void set_CV_add_flags(uint32_t a, uint32_t b, uint32_t result);
        void set_CV_sub_flags(uint32_t a, uint32_t b, uint32_t result);

        void spsr_to_cpsr();
    
        uint32_t lsl(uint32_t value, int shift, bool alter_flags);
        uint32_t lsl_32(uint32_t value, bool alter_flags);
        uint32_t lsr(uint32_t value, int shift, bool alter_flags);
        uint32_t lsr_32(uint32_t value, bool alter_flags);
        uint32_t asr(uint32_t value, int shift, bool alter_flags);
        uint32_t asr_32(uint32_t value, bool alter_flags);
        uint32_t rrx(uint32_t value, bool alter_flags);
    
        uint32_t rotr32(uint32_t n, unsigned int c, bool alter_flags);
};

//Inline getters/setters here
inline uint64_t ARM_CPU::get_timestamp() { return timestamp; }
inline int64_t ARM_CPU::cycles_ran() { return timestamp - last_timestamp; }
inline uint32_t ARM_CPU::get_PC() { return regs[15]; }
inline uint32_t ARM_CPU::get_current_instr() { return current_instr; }

inline uint32_t ARM_CPU::get_register(int id)
{
    return regs[id];
}

inline void ARM_CPU::set_register(int id, uint32_t value)
{
    regs[id] = value;
}

//inline uint32_t ARM_CPU::get_lo_register(int id) { return lo_gen_regs[id]; }
/*inline void ARM_CPU::set_lo_register(int id, uint32_t value)
{
    if (can_disassemble())
        printf("\nNew value for {%d}: $%08X", id, value);
    lo_gen_regs[id] = value;
}*/

inline void ARM_CPU::add_internal_cycles(int cycles) { timestamp += cycles; }
inline void ARM_CPU::add_cop_cycles(int cycles) { timestamp += cycles; }

inline void ARM_CPU::set_zero_neg_flags(uint32_t value)
{
    CPSR.zero = value == 0;
    CPSR.negative = (value & (1 << 31)) != 0;
}

#endif /* cpu_hpp */
