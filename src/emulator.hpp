/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef emulator_hpp
#define emulator_hpp
#include "gba/gbadma.hpp"
#include "bios.hpp"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
#include "dma.hpp"
#include "gpu.hpp"
#include "interrupts.hpp"
#include "ipc.hpp"
#include "rtc.hpp"
#include "slot2.hpp"
#include "spi.hpp"
#include "spu.hpp"
#include "timers.hpp"
#include "wifi.hpp"
#include "memconsts.h"

struct KEYINPUT_REG
{
    bool button_A;
    bool button_B;
    bool select;
    bool start;
    bool right;
    bool left;
    bool up;
    bool down;
    bool button_R;
    bool button_L;
    
    uint16_t get();
};

struct EXTKEYIN_REG
{
    bool button_X;
    bool button_Y;
    bool pen_down;
    bool hinge_closed;
    
    uint8_t get();
};

struct POWCNT2_REG
{
    bool sound_enabled;
    bool wifi_enabled;

    void set(uint8_t byte);
    uint8_t get();
};

struct SchedulerEvent
{
    int id;
    bool processing;
    uint64_t activation_time;
};

class Emulator
{
    private:
        uint64_t cycle_count;
        ARM_CPU arm7, arm9;
        BIOS bios;
        CP15 arm9_cp15;
        Debugger debugger;
        NDS_Cart cart;
        NDS_DMA dma;
        GPU gpu;
        RealTimeClock rtc;
        Slot2Device slot2;
        SPI_Bus spi;
        SPU spu;
        NDS_Timing timers;
        WiFi wifi;

        bool gba_mode;
        GBA_DMA gba_dma;
    
        uint8_t main_RAM[1024 * 1024 * 4]; //4 MB
        uint8_t shared_WRAM[1024 * 32]; //32 KB
        uint8_t arm7_WRAM[1024 * 64]; //64 KB
        uint8_t arm9_bios[BIOS9_SIZE];
        uint8_t arm7_bios[BIOS7_SIZE];
        uint8_t gba_bios[BIOS_GBA_SIZE];

        //Scheduling
        uint64_t system_timestamp;
        uint64_t next_event_time;
        SchedulerEvent GPU_event, DMA_event;
    
        IPCSYNC IPCSYNC_NDS9, IPCSYNC_NDS7;
        IPCFIFO fifo7, fifo9;
        std::queue<uint32_t> fifo7_queue, fifo9_queue;
        uint16_t AUXSPICNT;
    
        InterruptRegs int7_reg, int9_reg;
    
        KEYINPUT_REG KEYINPUT;
        EXTKEYIN_REG EXTKEYIN;

        POWCNT2_REG POWCNT2;
    
        uint32_t DMAFILL[4];
        uint16_t SIOCNT; //0x04000128
        uint16_t RCNT; //0x04000134
        uint16_t EXMEMCNT;
        uint8_t WRAMCNT; //0x04000247
        uint16_t DIVCNT; //0x04000280
        uint64_t DIV_NUMER;
        uint64_t DIV_DENOM;
        uint64_t DIV_RESULT;
        uint64_t DIV_REMRESULT;
        uint16_t SQRTCNT;
        uint32_t SQRT_RESULT;
        uint64_t SQRT_PARAM;
        uint8_t POSTFLG7, POSTFLG9; //0x04000300
        uint32_t BIOSPROT; //0x04000308

        uint16_t WAITCNT;
    
        bool hstep_even; //Debugging purposes
    
        int cycles = 0;

        uint64_t total_timestamp;
        uint64_t last_arm9_timestamp;
        uint64_t last_arm7_timestamp;

        void check_fifo7_interrupt();
        void check_fifo9_interrupt();

        void start_division();
        void start_sqrt();
    public:
        Emulator();
        int init();
        int load_firmware();
        void load_bios_gba(uint8_t* bios);
        void load_bios7(uint8_t* bios);
        void load_bios9(uint8_t* bios);
        void load_firmware(uint8_t* firmware);
        void load_slot2(uint8_t* data, uint64_t size);
        void load_save_database(std::string name);
        int load_ROM(std::string ROM_name);

        void mark_as_arm(uint32_t address);
        void mark_as_thumb(uint32_t address);

        void power_on();
        void direct_boot();
        void debug();
        void run();
        void run_gba();
        bool requesting_interrupt(int cpu_id);

        bool is_gba();
        void start_gba_mode(bool throw_exception);

        uint64_t get_timestamp();

        void get_upper_frame(uint32_t* buffer);
        void get_lower_frame(uint32_t* buffer);

        void set_upper_screen(uint32_t* buffer);
        void set_lower_screen(uint32_t* buffer);

        bool frame_complete();
        bool display_swapped();
        bool DMA_active(int cpu_id);
    
        void HBLANK_DMA_request();
        void gamecart_DMA_request();
        void GXFIFO_DMA_request();
        void check_GXFIFO_DMA();

        void add_GPU_event(int event_id, uint64_t relative_time);
        void add_DMA_event(int event_id, uint64_t relative_time);
        void calculate_system_timestamp();

        void touchscreen_press(int x, int y);
        int hle_bios(int cpu_id);
    
        uint32_t arm9_read_word(uint32_t address);
        uint16_t arm9_read_halfword(uint32_t address);
        uint8_t arm9_read_byte(uint32_t address);
        void arm9_write_word(uint32_t address, uint32_t word);
        void arm9_write_halfword(uint32_t address, uint16_t halfword);
        void arm9_write_byte(uint32_t address, uint8_t byte);
    
        uint32_t arm7_read_word(uint32_t address);
        uint16_t arm7_read_halfword(uint32_t address);
        uint8_t arm7_read_byte(uint32_t address);
        void arm7_write_word(uint32_t address, uint32_t word);
        void arm7_write_halfword(uint32_t address, uint16_t halfword);
        void arm7_write_byte(uint32_t address, uint8_t byte);

        uint32_t gba_read_word(uint32_t address);
        uint16_t gba_read_halfword(uint32_t address);
        uint8_t gba_read_byte(uint32_t address);
        void gba_write_word(uint32_t address, uint32_t word);
        void gba_write_halfword(uint32_t address, uint16_t halfword);
        void gba_write_byte(uint32_t address, uint8_t byte);
    
        void cart_copy_keybuffer(uint8_t* buffer);
        void cart_write_header(uint32_t address, uint16_t halfword);
    
        void request_interrupt7(INTERRUPT id);
        void request_interrupt9(INTERRUPT id);
        void request_interrupt_gba(int id);
    
        bool arm7_has_cart_rights();
    
        ARM_CPU* get_arm9();
        ARM_CPU* get_arm7();
        SPU* get_SPU();

        void button_up_pressed();
        void button_down_pressed();
        void button_left_pressed();
        void button_right_pressed();
        void button_start_pressed();
        void button_select_pressed();
        void button_a_pressed();
        void button_b_pressed();
        void button_x_pressed();
        void button_y_pressed();
        void button_l_pressed();
        void button_r_pressed();

        void button_up_released();
        void button_down_released();
        void button_left_released();
        void button_right_released();
        void button_start_released();
        void button_select_released();
        void button_a_released();
        void button_b_released();
        void button_x_released();
        void button_y_released();
        void button_l_released();
        void button_r_released();
};

inline void Emulator::set_upper_screen(uint32_t* buffer)
{
    gpu.set_upper_buffer(buffer);
}

inline void Emulator::set_lower_screen(uint32_t *buffer)
{
    gpu.set_lower_buffer(buffer);
}

bool inline Emulator::requesting_interrupt(int cpu_id)
{
    if (cpu_id)
        return (int7_reg.IE & int7_reg.IF) && (int7_reg.IME);
    return (int9_reg.IE & int9_reg.IF) && (int9_reg.IME);
}

bool inline Emulator::frame_complete()
{
    return gpu.is_frame_complete();
}

bool inline Emulator::display_swapped()
{
    return gpu.display_swapped();
}

bool inline Emulator::DMA_active(int cpu_id)
{
    return dma.is_active(cpu_id);
}

void inline Emulator::button_up_pressed()
{
    KEYINPUT.up = true;
}

void inline Emulator::button_down_pressed()
{
    KEYINPUT.down = true;
}

void inline Emulator::button_left_pressed()
{
    KEYINPUT.left = true;
}

void inline Emulator::button_right_pressed()
{
    KEYINPUT.right = true;
}

void inline Emulator::button_a_pressed()
{
    KEYINPUT.button_A = true;
}

void inline Emulator::button_b_pressed()
{
    KEYINPUT.button_B = true;
}

void inline Emulator::button_x_pressed()
{
    EXTKEYIN.button_X = true;
}

void inline Emulator::button_y_pressed()
{
    EXTKEYIN.button_Y = true;
}

void inline Emulator::button_l_pressed()
{
    KEYINPUT.button_L = true;
}

void inline Emulator::button_r_pressed()
{
    KEYINPUT.button_R = true;
}

void inline Emulator::button_start_pressed()
{
    KEYINPUT.start = true;
}

void inline Emulator::button_select_pressed()
{
    KEYINPUT.select = true;
}

void inline Emulator::button_up_released()
{
    KEYINPUT.up = false;
}

void inline Emulator::button_down_released()
{
    KEYINPUT.down = false;
}

void inline Emulator::button_left_released()
{
    KEYINPUT.left = false;
}

void inline Emulator::button_right_released()
{
    KEYINPUT.right = false;
}

void inline Emulator::button_a_released()
{
    KEYINPUT.button_A = false;
}

void inline Emulator::button_b_released()
{
    KEYINPUT.button_B = false;
}

void inline Emulator::button_x_released()
{
    EXTKEYIN.button_X = false;
}

void inline Emulator::button_y_released()
{
    EXTKEYIN.button_Y = false;
}

void inline Emulator::button_l_released()
{
    KEYINPUT.button_L = false;
}

void inline Emulator::button_r_released()
{
    KEYINPUT.button_R = false;
}

void inline Emulator::button_start_released()
{
    KEYINPUT.start = false;
}

void inline Emulator::button_select_released()
{
    KEYINPUT.select = false;
}

#endif /* emulator_hpp */
