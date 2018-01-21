/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include "emulator.hpp"
#include "config.hpp"
#include "disassembler.hpp"

using namespace std;

uint16_t KEYINPUT_REG::get()
{
    uint16_t reg = 0x3FF;
    reg &= ~button_A;
    reg &= ~(button_B << 1);
    reg &= ~(select << 2);
    reg &= ~(start << 3);
    reg &= ~(right << 4);
    reg &= ~(left << 5);
    reg &= ~(up << 6);
    reg &= ~(down << 7);
    reg &= ~(button_R << 8);
    reg &= ~(button_L << 9);
    return reg;
}

uint8_t EXTKEYIN_REG::get()
{
    uint8_t reg = 0x7F;
    reg &= ~button_X;
    reg &= ~(button_Y << 1);
    reg &= ~(pen_down << 6);
    reg |= (hinge_closed << 7);
    return reg;
}

uint8_t POWCNT2_REG::get()
{
    uint8_t reg = 0;
    reg |= sound_enabled;
    reg |= wifi_enabled << 1;
    return reg;
}

void POWCNT2_REG::set(uint8_t byte)
{
    sound_enabled = byte & 0x1;
    wifi_enabled = byte & 0x2;
}

Emulator::Emulator() : arm7(this, 1), arm9(this, 0), arm9_cp15(this), cart(this), debugger(this), dma(this),
                       gba_dma(this), gpu(this), spi(this), spu(this), timers(this) {}

int Emulator::init()
{
    arm9.set_cp15(&arm9_cp15);

    fifo7.receive_queue = &fifo7_queue;
    fifo7.send_queue = &fifo9_queue;

    fifo9.receive_queue = &fifo9_queue;
    fifo9.send_queue = &fifo7_queue;
    
    return 0;
}

void Emulator::power_on()
{
    for (int i = 0; i < 4; i++)
        Config::bg_enable[i] = true;
    gba_mode = false;
    cycle_count = 0;
    arm9.power_on();
    arm7.power_on();
    arm9_cp15.power_on();
    dma.power_on();
    gpu.power_on();
    spu.power_on();
    timers.power_on();
    rtc.init();
    total_timestamp = 20; //Give the processors some time to run
    POWCNT2.sound_enabled = true;
    POWCNT2.wifi_enabled = false;
    
    system_timestamp = 0;
    next_event_time = 0xFFFFFFFF;
    GPU_event.activation_time = 0;
    GPU_event.id = 0;
    DMA_event.activation_time = 0;
    DMA_event.processing = false;
    DMA_event.id = 0;

    POSTFLG7 = 0;
    POSTFLG9 = 0;
    AUXSPICNT = 0;
    SIOCNT = 0;
    BIOSPROT = 0;
    hstep_even = true;

    SQRTCNT = 0;
    DIVCNT = 0;
    
    KEYINPUT.button_A = false;
    KEYINPUT.button_B = false;
    KEYINPUT.select = false;
    KEYINPUT.start = false;
    KEYINPUT.right = false;
    KEYINPUT.left = false;
    KEYINPUT.up = false;
    KEYINPUT.down = false;
    KEYINPUT.button_R = false;
    KEYINPUT.button_L = false;
    
    EXTKEYIN.button_X = false;
    EXTKEYIN.button_Y = false;
    EXTKEYIN.pen_down = false;
    EXTKEYIN.hinge_closed = false;
    
    IPCSYNC_NDS7.input = 0;
    IPCSYNC_NDS9.input = 0;
    fifo7.write_CNT(0);
    fifo7.error = false;
    fifo9.write_CNT(0);
    fifo9.error = false;
    fifo7.recent_word = 0;
    fifo9.recent_word = 0;
    
    memset(main_RAM, 0x0, 1024 * 1024 * 4);
    memset(shared_WRAM, 0x0, 1024 * 32);
    memset(arm7_WRAM, 0x0, 1024 * 64);

    int7_reg.IME = 0;
    int7_reg.IE = 0;
    int7_reg.IF = 0;
    int9_reg.IME = 0;
    int9_reg.IE = 0;
    int9_reg.IF = 0;

    if (Config::direct_boot_enabled && cart.game_loaded())
        direct_boot();
}

void Emulator::load_save_database(std::string name)
{
    cart.load_database(name);
}

int Emulator::load_ROM(string ROM_file_name)
{
    if (cart.load_ROM(ROM_file_name))
        return 1;
    power_on();
    return 0;
}

void Emulator::load_bios7(uint8_t *bios)
{
    memcpy(arm7_bios, bios, BIOS7_SIZE);
}

void Emulator::load_bios9(uint8_t *bios)
{
    memcpy(arm9_bios, bios, BIOS9_SIZE);
}

void Emulator::load_bios_gba(uint8_t *bios)
{
    memcpy(gba_bios, bios, BIOS_GBA_SIZE);
}

void Emulator::load_slot2(uint8_t *data, uint64_t size)
{
    slot2.load_data(data, size);
}

void Emulator::load_firmware(uint8_t *firmware)
{
    spi.init(firmware);
}

int Emulator::load_firmware()
{
    ifstream arm9_bios_file(Config::arm9_bios_path, ios::in | ios::binary);
    
    if (!arm9_bios_file.is_open())
    {
        printf("Failed to load ARM9 BIOS\n");
        return 1;
    }
    
    arm9_bios_file.read((char*)arm9_bios, BIOS9_SIZE);
    printf("ARM9 BIOS loaded successfully.\n");
    
    arm9_bios_file.close();
    
    ifstream arm7_bios_file(Config::arm7_bios_path, ios::in | ios::binary);
    
    if (!arm7_bios_file.is_open())
    {
        printf("Failed to load ARM7 BIOS\n");
        return 1;
    }
    
    arm7_bios_file.read((char*)arm7_bios, BIOS7_SIZE);
    printf("ARM7 BIOS loaded successfully.\n");
    
    arm7_bios_file.close();

    ifstream gba_bios_file(Config::gba_bios_path, ios::in | ios::binary);
    if (!gba_bios_file.is_open())
    {
        printf("Warning: failed to load GBA BIOS\n");
    }
    else
    {
        gba_bios_file.read((char*)gba_bios, BIOS_GBA_SIZE);
        gba_bios_file.close();
        printf("GBA BIOS loaded successfully.\n");
    }

    return spi.init(Config::firmware_path);
}

void Emulator::direct_boot()
{
    uint32_t boot_info[8];
    for (unsigned int i = 0; i < 0x200; i++)
        cart_write_header(i, cart.direct_read(i));
    //meh
    for (unsigned int i = 0; i < 8; i++)
        boot_info[i] = arm9_read_word(0x27FFE20 + (i * 4));
    
    printf("\nA9 offset: $%08X entry: $%08X RAM: $%08X size: $%08X", boot_info[0], boot_info[1], boot_info[2], boot_info[3]);
    printf("\nA7 offset: $%08X entry: $%08X RAM: $%08X size: $%08X\n", boot_info[4], boot_info[5], boot_info[6], boot_info[7]);
    
    //Initialize CPUs and regs to after-boot values
    arm9.direct_boot(boot_info[1]);
    arm7.direct_boot(boot_info[5]);

    //Write the ROM chip-id into main RAM
    arm7_write_word(0x027FF800, 0x00003FC2);
    arm7_write_word(0x027FF804, 0x00003FC2);
    arm7_write_word(0x027FFC00, 0x00003FC2);
    arm7_write_word(0x027FFC04, 0x00003FC2);

    //other bullshit
    arm7_write_halfword(0x027FF808, cart.direct_read_halfword(0x015E));
    arm7_write_halfword(0x027FF80A, cart.direct_read_halfword(0x006C));
    arm7_write_halfword(0x027FF850, 0x5835);

    arm7_write_halfword(0x027FFC08, cart.direct_read_halfword(0x015E));
    arm7_write_halfword(0x027FFC0A, cart.direct_read_halfword(0x006C));

    arm7_write_halfword(0x027FFC10, 0x5835);
    arm7_write_halfword(0x027FFC30, 0xFFFF);
    arm7_write_halfword(0x027FFC40, 0x0001);
    
    POSTFLG7 = 1;
    POSTFLG9 = 1;
    RCNT = 0x8000;
    
    BIOSPROT = 0x1204;
    WRAMCNT = 3;
    
    //Load ROM into RAM
    for (unsigned int i = 0; i < boot_info[3]; i += 4)
        arm9_write_word(boot_info[2] + i, cart.direct_read_word(boot_info[0] + i));
    
    for (unsigned int i = 0; i < boot_info[7]; i += 4)
        arm7_write_word(boot_info[6] + i, cart.direct_read_word(boot_info[4] + i));

    spi.direct_boot();
    
    cycles = 0;
}

void Emulator::mark_as_arm(uint32_t address)
{
    debugger.mark_as_arm(address);
}

void Emulator::mark_as_thumb(uint32_t address)
{
    debugger.mark_as_thumb(address);
}

void Emulator::debug()
{
    //printf("\nIE9: $%08X IF9: $%08X", int9_reg.IE, int9_reg.IF);
    //printf("\nIE7: $%08X IF7: $%08X", int7_reg.IE, int7_reg.IF);
    //debugger.dump_disassembly();
    Config::test = !Config::test;
}

void Emulator::run()
{
    gpu.start_frame();
    while (!gpu.is_frame_complete())
    {
        //Handle ARM9
        calculate_system_timestamp();
        while (arm9.get_timestamp() < (system_timestamp << 1))
        {
            arm9.execute();
            timers.run_timers9(arm9.cycles_ran() >> 1);
            gpu.run_3D(arm9.cycles_ran() >> 1);
        }
        uint64_t arm7_diff = arm7.get_timestamp();
        //Now handle ARM7
        while (arm7.get_timestamp() < system_timestamp)
        {
            arm7.execute();
            timers.run_timers7(arm7.cycles_ran());
        }

        arm7_diff = arm7.get_timestamp() - arm7_diff;

        spu.generate_sample(arm7_diff);

        if (system_timestamp >= GPU_event.activation_time)
            gpu.handle_event(GPU_event);

        if (system_timestamp >= DMA_event.activation_time && DMA_event.processing)
            dma.handle_event(DMA_event);

        cart.run(8);
    }
    cart.save_check();
}

void Emulator::run_gba()
{
    gpu.start_frame();
    while (!gpu.is_frame_complete())
    {
        arm7.execute();
        uint64_t cycle_count = arm7.cycles_ran();

        gpu.gba_run(cycle_count);
    }
}

bool Emulator::is_gba()
{
    return gba_mode;
}

//Only use throw_exception when emulation has started
void Emulator::start_gba_mode(bool throw_exception)
{
    power_on();
    WAITCNT = 0;
    gba_mode = true;
    arm7.gba_boot(true);
    //debug();
    //Allocate VRAM C and D as 256 KB work RAM
    gpu.set_VRAMCNT_C(0x82);
    gpu.set_VRAMCNT_D(0x8A);
    if (throw_exception)
    {
        //Signal to the emulation thread that emulation has switched from NDS to GBA
        throw "!";
    }
}

uint64_t Emulator::get_timestamp()
{
    return system_timestamp;
}

void Emulator::HBLANK_DMA_request()
{
    if (!gba_mode)
        dma.HBLANK_request();
    else
        gba_dma.HBLANK_request();
}

void Emulator::gamecart_DMA_request()
{
    dma.gamecart_request();
}

void Emulator::GXFIFO_DMA_request()
{
    dma.GXFIFO_request();
}

void Emulator::check_GXFIFO_DMA()
{
    gpu.check_GXFIFO_DMA();
}

void Emulator::add_GPU_event(int event_id, uint64_t relative_time)
{
    GPU_event.id = event_id;
    GPU_event.activation_time = system_timestamp + relative_time;
    if (GPU_event.activation_time < next_event_time)
        next_event_time = GPU_event.activation_time;
}

void Emulator::add_DMA_event(int event_id, uint64_t relative_time)
{
    DMA_event.id = event_id;
    DMA_event.processing = true;
    DMA_event.activation_time = system_timestamp + relative_time;
    if (DMA_event.activation_time < next_event_time)
        next_event_time = DMA_event.activation_time;
}

void Emulator::calculate_system_timestamp()
{
    int cycles = next_event_time - system_timestamp;
    if (cycles <= 0 || cycles > 20)
        system_timestamp += 20;
    else
        system_timestamp += cycles;
}

void Emulator::touchscreen_press(int x, int y)
{
    EXTKEYIN.pen_down = (y != 0xFFF);
    spi.touchscreen_press(x, y);
}

int Emulator::hle_bios(int cpu_id)
{
    if (cpu_id)
        return bios.SWI7(arm7);
    return bios.SWI9(arm9);
}

void Emulator::cart_copy_keybuffer(uint8_t *buffer)
{
    memcpy(buffer, arm7_bios + 0x30, 0x1048);
}

void Emulator::cart_write_header(uint32_t address, uint16_t halfword)
{
    *(uint16_t*)&main_RAM[(0x027FFE00 + (address & 0x1FF)) & MAIN_RAM_MASK] = halfword;
}

void Emulator::request_interrupt7(INTERRUPT id)
{
    //printf("\nRequesting interrupt7 %d", static_cast<int>(id));
    int7_reg.IF |= 1 << static_cast<int>(id);
}

void Emulator::request_interrupt9(INTERRUPT id)
{
    //printf("\nRequesting interrupt9 %d", static_cast<int>(id));
    int9_reg.IF |= 1 << static_cast<int>(id);
}

void Emulator::request_interrupt_gba(int id)
{
    int7_reg.IF |= 1 << id;
}

void Emulator::get_upper_frame(uint32_t* buffer)
{
    gpu.get_upper_frame(buffer);
}

void Emulator::get_lower_frame(uint32_t* buffer)
{
    gpu.get_lower_frame(buffer);
}

ARM_CPU* Emulator::get_arm9()
{
    return &arm9;
}

ARM_CPU* Emulator::get_arm7()
{
    return &arm7;
}

SPU* Emulator::get_SPU()
{
    return &spu;
}

bool Emulator::arm7_has_cart_rights()
{
    return EXMEMCNT & (1 << 11);
}

void Emulator::start_division()
{
    //TODO: emulate cycle timings
    //This will require a proper scheduling infrastructure

    //Clear busy and division by zero flags
    DIVCNT &= ~(1 << 15);
    DIVCNT &= ~(1 << 14);

    int mode = DIVCNT & 0x3;

    if (DIV_DENOM == 0)
        DIVCNT |= (1 << 14);

    switch (mode)
    {
        case 0:
            {
                int32_t numerator = static_cast<int32_t>(DIV_NUMER & 0xFFFFFFFF);
                int32_t denominator = static_cast<int32_t>(DIV_DENOM & 0xFFFFFFFF);

                //Div by zero
                if (denominator == 0)
                {
                    DIV_REMRESULT = static_cast<int64_t>(numerator);
                    DIV_RESULT = (numerator >= 0) ? -1 : 1;
                }
                //-MAX/-1 overflow
                else if (numerator == -(int32_t)0x80000000 && denominator == -1)
                {
                    DIV_RESULT = 0x80000000;
                }
                else
                {
                    DIV_RESULT = static_cast<int64_t>(numerator / denominator);
                    DIV_REMRESULT = static_cast<int64_t>(numerator % denominator);
                }
            }
            break;
        case 1:
        case 3: //Mode 3 is prohibited but has the same behavior as mode 1
            {
                int64_t numerator = static_cast<int64_t>(DIV_NUMER);
                int32_t denominator = static_cast<int32_t>(DIV_DENOM & 0xFFFFFFFF);
                if (denominator == 0)
                {
                    DIV_REMRESULT = static_cast<int64_t>(numerator);
                    DIV_RESULT = (numerator >= 0) ? -1 : 1;
                }
                else if (numerator == -(int64_t)0x8000000000000000 && denominator == -1)
                {
                    DIV_RESULT = 0x8000000000000000;
                }
                else
                {
                    DIV_RESULT = static_cast<int64_t>(numerator / denominator);
                    DIV_REMRESULT = static_cast<int64_t>(numerator % denominator);
                }
            }
            break;
        case 2:
            {
                int64_t numerator = static_cast<int64_t>(DIV_NUMER);
                int64_t denominator = static_cast<int64_t>(DIV_DENOM);
                if (denominator == 0)
                {
                    DIV_REMRESULT = static_cast<int64_t>(numerator);
                    DIV_RESULT = (numerator >= 0) ? -1 : 1;
                }
                else if (numerator == -(int64_t)0x8000000000000000 && denominator == -1)
                {
                    DIV_RESULT = 0x8000000000000000;
                }
                else
                {
                    DIV_RESULT = static_cast<int64_t>(numerator / denominator);
                    DIV_REMRESULT = static_cast<int64_t>(numerator % denominator);
                }
            }
            break;
        default:
            printf("\nUnrecogized division mode %d", mode);
    }
    //printf("\nDivision: %lld / %lld = %lld (%lld)", DIV_NUMER, DIV_DENOM, DIV_RESULT, DIV_REMRESULT);
}

void Emulator::start_sqrt()
{
    uint64_t x = SQRT_PARAM;
    if (!(SQRTCNT & 0x1))
        x &= 0xFFFFFFFF; //32-bit mode

    SQRTCNT &= ~(1 << 15);

    //TODO: check me?
    SQRT_RESULT = static_cast<uint32_t>(sqrt(x));
}
