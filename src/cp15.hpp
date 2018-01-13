/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef cp15_hpp
#define cp15_hpp
#include <cstdlib>
#include <cstdint>

struct ControlReg
{
    bool mmu_pu_enable;
    bool data_unified_cache_on;
    bool is_big_endian;
    bool instruction_cache_on;
    bool high_exception_vector;
    bool predictable_cache_replacement;
    bool pre_ARMv5_mode;
    bool dtcm_enable;
    bool dtcm_write_only;
    bool itcm_enable;
    bool itcm_write_only;
    
    void set_values(uint32_t reg);
    uint32_t get_values();
};

class ARM_CPU;
class Emulator;

class CP15
{
    private:
        Emulator* e;
        ARM_CPU* arm9;
        ControlReg control;
    
        uint32_t itcm_data;
        uint32_t dtcm_data;

        //So we don't have to re-calculate every time the ARM9 accesses memory
        uint32_t itcm_size;
        uint32_t dtcm_base;
        uint32_t dtcm_size;

        uint8_t ITCM[1024 * 32];
        uint8_t DTCM[1024 * 16];

        uint8_t dcache[1024 * 4];
        uint8_t icache[1024 * 8];
    public:
        CP15(Emulator* e);
    
        void power_on();
        void link_with_cpu(ARM_CPU* arm9);
    
        uint32_t get_itcm_size();
        uint32_t get_dtcm_base();
        uint32_t get_dtcm_size();

        bool dtcm_write_only();

        uint32_t read_word(uint32_t address);
        uint16_t read_halfword(uint32_t address);
        uint8_t read_byte(uint32_t address);

        void write_word(uint32_t address, uint32_t word);
        void write_halfword(uint32_t address, uint16_t halfword);
        void write_byte(uint32_t address, uint32_t byte);
    
        void mcr(int operation, int destination_reg, int ARM_reg_contents, int info, int operand_reg);
        uint32_t mrc(int operation, int source_reg, int info, int operand_reg);
};

inline bool CP15::dtcm_write_only()
{
    return control.dtcm_write_only;
}

inline uint32_t CP15::get_itcm_size()
{
    if (control.itcm_enable)
        return itcm_size;
    return 0;
}

inline uint32_t CP15::get_dtcm_base()
{
    return dtcm_base;
}

inline uint32_t CP15::get_dtcm_size()
{
    if (control.dtcm_enable)
        return dtcm_size;
    return 0;
}

#endif /* cp15_hpp */
