/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef GPU_HPP
#define GPU_HPP
#include <cstdint>
#include <cstdlib>
#include "gpu3d.hpp"
#include "gpueng.hpp"
#include "memconsts.h"

//Defines for GBA mode
#define GBA_BG VRAM_E
#define GBA_OBJ VRAM_F

struct DISPSTAT_REG
{
    bool is_VBLANK;
    bool is_HBLANK;
    bool is_VCOUNTER;
    bool IRQ_on_VBLANK;
    bool IRQ_on_HBLANK;
    bool IRQ_on_VCOUNTER;
    int VCOUNTER;
};

struct VRAM_BANKCNT
{
    int MST;
    uint32_t offset;
    bool enabled;
};

struct POWCNT1_REG
{
    bool lcd_enable;
    bool engine_a;
    bool rendering_3d;
    bool geometry_3d;
    bool engine_b;
    bool swap_display;
};

class Emulator;
struct SchedulerEvent;

class GPU
{
    private:
        Emulator* e;
        GPU_2D_Engine eng_A, eng_B;
        GPU_3D eng_3D;

        bool frame_complete;
        int frames_skipped;

        uint64_t cycles;

        uint8_t VRAM_A[VRAM_A_SIZE];
        uint8_t VRAM_B[VRAM_B_SIZE];
        uint8_t VRAM_C[VRAM_C_SIZE];
        uint8_t VRAM_D[VRAM_D_SIZE];
        uint8_t VRAM_E[VRAM_E_SIZE];
        uint8_t VRAM_F[VRAM_F_SIZE];
        uint8_t VRAM_G[VRAM_G_SIZE];
        uint8_t VRAM_H[VRAM_H_SIZE];
        uint8_t VRAM_I[VRAM_I_SIZE];

        uint8_t palette_A[1024];
        uint8_t palette_B[1024];

        uint8_t OAM[1024 * 2];

        DISPSTAT_REG DISPSTAT7, DISPSTAT9;

        uint16_t VCOUNT; //0x4000006

        VRAM_BANKCNT VRAMCNT_A, VRAMCNT_B, VRAMCNT_C, VRAMCNT_D, VRAMCNT_E;
        VRAM_BANKCNT VRAMCNT_F, VRAMCNT_G, VRAMCNT_H, VRAMCNT_I;

        POWCNT1_REG POWCNT1;

        void draw_bg_txt_line(int index, bool engine_a);
        void draw_bg_extended_line(int index, bool engine_a);

        void draw_sprite_line(bool engine_a);

        void draw_scanline();
    public:
        GPU(Emulator* e);

        uint64_t get_cycles() { return cycles; }

        void power_on();
        void run_3D(uint64_t cycles);
        void handle_event(SchedulerEvent& event);

        void get_upper_frame(uint32_t* buffer);
        void get_lower_frame(uint32_t* buffer);

        void start_frame();
        void end_frame();
        void check_GXFIFO_DMA();
        void check_GXFIFO_IRQ();
        bool is_frame_complete();
        bool display_swapped();
        uint16_t read_palette_A(uint32_t address);
        uint16_t read_extpal_bga(uint32_t address);
        uint16_t read_extpal_obja(uint32_t address);
        uint16_t read_palette_B(uint32_t address);
        uint16_t read_extpal_bgb(uint32_t address);
        uint16_t read_extpal_objb(uint32_t address);

        template <typename T> T read_bga(uint32_t address);
        template <typename T> T read_bgb(uint32_t address);
        template <typename T> T read_obja(uint32_t address);
        template <typename T> T read_objb(uint32_t address);
        template <typename T> T read_teximage(uint32_t address);
        template <typename T> T read_texpal(uint32_t address);
        template <typename T> T read_lcdc(uint32_t address);
        template <typename T> T read_OAM(uint32_t address);
        void write_palette_A(uint32_t address, uint16_t halfword);
        void write_palette_B(uint32_t address, uint16_t halfword);
        void write_bga(uint32_t address, uint16_t halfword);
        void write_bgb(uint32_t address, uint16_t halfword);
        void write_obja(uint32_t address, uint16_t halfword);
        void write_objb(uint32_t address, uint16_t halfword);
        void write_lcdc(uint32_t address, uint16_t halfword);
        void write_OAM(uint32_t address, uint16_t halfword);

        template <typename T> T read_ARM7(uint32_t address);
        template <typename T> T read_gba(uint32_t address);
        template <typename T> void write_ARM7(uint32_t address, T value);
        template <typename T> void write_gba(uint32_t address, T value);

        uint16_t* get_palette(bool engine_A);
        uint16_t* get_VRAM_block(int id);

        uint32_t get_DISPCNT_A();
        uint32_t get_DISPCNT_B();

        uint16_t get_DISPSTAT7();
        uint16_t get_DISPSTAT9();

        uint16_t get_BGCNT_A(int index);
        uint16_t get_BGCNT_B(int index);

        uint16_t get_VCOUNT();

        uint16_t get_BGHOFS_A(int index);
        uint16_t get_BGVOFS_A(int index);
        uint16_t get_BGHOFS_B(int index);
        uint16_t get_BGVOFS_B(int index);

        uint32_t get_BG2X_A();

        uint16_t get_WIN0V_A();
        uint16_t get_WIN1V_A();
        uint16_t get_WIN0V_B();
        uint16_t get_WIN1V_B();

        uint16_t get_WININ_A();
        uint16_t get_WININ_B();

        uint16_t get_WINOUT_A();
        uint16_t get_WINOUT_B();

        uint16_t get_BLDCNT_A();
        uint16_t get_BLDCNT_B();
        uint16_t get_BLDALPHA_A();
        uint16_t get_BLDALPHA_B();

        uint16_t get_DISP3DCNT();

        uint16_t get_MASTER_BRIGHT_A();
        uint16_t get_MASTER_BRIGHT_B();
        uint32_t get_DISPCAPCNT();

        uint8_t get_VRAMSTAT();
        uint8_t get_VRAMCNT_A();
        uint8_t get_VRAMCNT_B();
        uint8_t get_VRAMCNT_C();
        uint8_t get_VRAMCNT_D();
        uint8_t get_VRAMCNT_E();
        uint8_t get_VRAMCNT_F();
        uint8_t get_VRAMCNT_G();
        uint8_t get_VRAMCNT_H();
        uint8_t get_VRAMCNT_I();

        uint16_t get_POWCNT1();

        uint32_t get_GXSTAT();
        uint16_t get_vert_count();
        uint16_t get_poly_count();
        uint16_t read_vec_test(uint32_t address);
        uint32_t read_clip_mtx(uint32_t address);
        uint32_t read_vec_mtx(uint32_t address);

        void set_upper_buffer(uint32_t* buffer);
        void set_lower_buffer(uint32_t* buffer);

        void set_DISPCNT_A_lo(uint16_t halfword);
        void set_DISPCNT_A(uint32_t word);

        void set_DISPCNT_B_lo(uint16_t halfword);
        void set_DISPCNT_B(uint32_t word);

        void set_DISPSTAT7(uint16_t halfword);
        void set_DISPSTAT9(uint16_t halfword);

        void set_BGCNT_A(uint16_t halfword, int index);
        void set_BGCNT_B(uint16_t halfword, int index);

        void set_BGHOFS_A(uint16_t halfword, int index);
        void set_BGVOFS_A(uint16_t halfword, int index);
        void set_BGHOFS_B(uint16_t halfword, int index);
        void set_BGVOFS_B(uint16_t halfword, int index);

        void set_BG2P_A(uint16_t halfword, int index);
        void set_BG2P_B(uint16_t halfword, int index);
        void set_BG3P_A(uint16_t halfword, int index);
        void set_BG3P_B(uint16_t halfword, int index);

        void set_BG2X_A(uint32_t word);
        void set_BG2Y_A(uint32_t word);
        void set_BG3X_A(uint32_t word);
        void set_BG3Y_A(uint32_t word);
        void set_BG2X_B(uint32_t word);
        void set_BG2Y_B(uint32_t word);
        void set_BG3X_B(uint32_t word);
        void set_BG3Y_B(uint32_t word);

        void set_WIN0H_A(uint16_t halfword);
        void set_WIN1H_A(uint16_t halfword);
        void set_WIN0V_A(uint16_t halfword);
        void set_WIN1V_A(uint16_t halfword);
        void set_WIN0H_B(uint16_t halfword);
        void set_WIN1H_B(uint16_t halfword);
        void set_WIN0V_B(uint16_t halfword);
        void set_WIN1V_B(uint16_t halfword);

        void set_WININ_A(uint16_t halfword);
        void set_WININ_B(uint16_t halfword);
        void set_WINOUT_A(uint16_t halfword);
        void set_WINOUT_B(uint16_t halfword);

        void set_MOSAIC_A(uint16_t halfword);
        void set_MOSAIC_B(uint16_t halfword);

        void set_BLDCNT_A(uint16_t halfword);
        void set_BLDCNT_B(uint16_t halfword);

        void set_BLDALPHA_A(uint16_t halfword);
        void set_BLDALPHA_B(uint16_t halfword);

        void set_BLDY_A(uint8_t byte);
        void set_BLDY_B(uint8_t byte);

        void set_DISP3DCNT(uint16_t halfword);

        void set_MASTER_BRIGHT_A(uint16_t halfword);
        void set_MASTER_BRIGHT_B(uint16_t halfword);
        void set_DISPCAPCNT(uint32_t word);

        void set_VRAMCNT_A(uint8_t byte);
        void set_VRAMCNT_B(uint8_t byte);
        void set_VRAMCNT_C(uint8_t byte);
        void set_VRAMCNT_D(uint8_t byte);
        void set_VRAMCNT_E(uint8_t byte);
        void set_VRAMCNT_F(uint8_t byte);
        void set_VRAMCNT_G(uint8_t byte);
        void set_VRAMCNT_H(uint8_t byte);
        void set_VRAMCNT_I(uint8_t byte);

        void set_POWCNT1(uint16_t value);

        void write_GXFIFO(uint32_t word);
        void write_FIFO_direct(uint32_t address, uint32_t word);

        void set_CLEAR_COLOR(uint32_t word);
        void set_CLEAR_DEPTH(uint32_t word);
        void set_EDGE_COLOR(uint32_t address, uint16_t halfword);
        void set_FOG_COLOR(uint32_t word);
        void set_FOG_OFFSET(uint16_t halfword);
        void set_FOG_TABLE(uint32_t address, uint8_t byte);
        void set_MTX_MODE(uint32_t word);
        void MTX_PUSH();
        void MTX_POP(uint32_t word);
        void MTX_IDENTITY();
        void MTX_MULT_4x4(uint32_t word);
        void MTX_MULT_4x3(uint32_t word);
        void MTX_MULT_3x3(uint32_t word);
        void MTX_TRANS(uint32_t word);
        void COLOR(uint32_t word);
        void VTX_16(uint32_t word);
        void set_POLYGON_ATTR(uint32_t word);
        void set_TEXIMAGE_PARAM(uint32_t word);
        void set_TOON_TABLE(uint32_t address, uint16_t color);
        void BEGIN_VTXS(uint32_t word);
        void SWAP_BUFFERS(uint32_t word);
        void VIEWPORT(uint32_t word);
        void set_GXSTAT(uint32_t word);

        void gba_run(uint64_t cycles);
        void gba_set_DISPCNT(uint16_t halfword);
};

template <typename T>
T GPU::read_bga(uint32_t address)
{
    T reg = 0;
    if (VRAMCNT_A.enabled && VRAMCNT_A.MST == 1)
    {
        if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_A.offset * 0x20000), VRAM_A_SIZE))
            reg |= *(T*)&VRAM_A[address & VRAM_A_MASK];
    }
    if (VRAMCNT_B.enabled && VRAMCNT_B.MST == 1)
    {
        if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_B.offset * 0x20000), VRAM_B_SIZE))
            reg |= *(T*)&VRAM_B[address & VRAM_B_MASK];
    }
    if (VRAMCNT_C.enabled && VRAMCNT_C.MST == 1)
    {
        if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_C.offset * 0x20000), VRAM_C_SIZE))
            reg |= *(T*)&VRAM_C[address & VRAM_C_MASK];
    }
    if (VRAMCNT_D.enabled && VRAMCNT_D.MST == 1)
    {
        if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_D.offset * 0x20000), VRAM_D_SIZE))
            reg |= *(T*)&VRAM_D[address & VRAM_D_MASK];
    }
    if (VRAMCNT_E.enabled && VRAMCNT_E.MST == 1)
    {
        if (ADDR_IN_RANGE(VRAM_BGA_START, VRAM_E_SIZE))
            reg |= *(T*)&VRAM_E[address & VRAM_E_MASK];
    }
    if (VRAMCNT_F.enabled && VRAMCNT_F.MST == 1)
    {
        uint32_t f_offset = (VRAMCNT_F.offset & 0x1) * 0x4000 + (VRAMCNT_F.offset & 0x2) * 0x8000;
        if (ADDR_IN_RANGE(VRAM_BGA_START + f_offset, VRAM_F_SIZE))
            reg |= *(T*)&VRAM_F[address & VRAM_F_MASK];
    }
    if (VRAMCNT_G.enabled && VRAMCNT_G.MST == 1)
    {
        uint32_t g_offset = (VRAMCNT_G.offset & 0x1) * 0x4000 + (VRAMCNT_G.offset & 0x2) * 0x8000;
        if (ADDR_IN_RANGE(VRAM_BGA_START + g_offset, VRAM_G_SIZE))
            reg |= *(T*)&VRAM_G[address & VRAM_G_MASK];
    }
    return reg;
}

template <typename T>
T GPU::read_bgb(uint32_t address)
{
    T value = 0;
    if (ADDR_IN_RANGE(VRAM_BGB_C, VRAM_C_SIZE) && VRAMCNT_C.MST == 4)
    {
        if (VRAMCNT_C.enabled)
            value |= *(T*)&VRAM_C[address & VRAM_C_MASK];
    }
    if (ADDR_IN_RANGE(VRAM_BGB_H, VRAM_H_SIZE) && VRAMCNT_H.MST == 1)
    {
        if (VRAMCNT_H.enabled)
            value |= *(T*)&VRAM_H[address & VRAM_H_MASK];
    }

    if (ADDR_IN_RANGE(VRAM_BGB_I, VRAM_I_SIZE) && VRAMCNT_I.MST == 1)
    {
        if (VRAMCNT_I.enabled)
            value |= *(T*)&VRAM_I[address & VRAM_I_MASK];
    }
    return value;
}

template <typename T>
T GPU::read_obja(uint32_t address)
{
    T reg = 0;
    if (ADDR_IN_RANGE(VRAM_OBJA_START + ((VRAMCNT_A.offset & 0x1) * 0x20000), VRAM_A_SIZE) && VRAMCNT_A.MST == 2)
        reg |= *(T*)&VRAM_A[address & VRAM_A_MASK];
    if (ADDR_IN_RANGE(VRAM_OBJA_START + ((VRAMCNT_B.offset & 0x1) * 0x20000), VRAM_B_SIZE) && VRAMCNT_B.MST == 2)
        reg |= *(T*)&VRAM_B[address & VRAM_B_MASK];
    if (ADDR_IN_RANGE(VRAM_OBJA_START, VRAM_E_SIZE) && VRAMCNT_E.MST == 2)
        reg |= *(T*)&VRAM_E[address & VRAM_E_MASK];
    uint32_t f_offset = (VRAMCNT_F.offset & 0x1) * 0x4000 + (VRAMCNT_F.offset & 0x2) * 0x8000;
    if (ADDR_IN_RANGE(VRAM_OBJA_START + f_offset, VRAM_F_SIZE) && VRAMCNT_F.MST == 2)
        reg |= *(T*)&VRAM_F[address & VRAM_F_MASK];
    uint32_t g_offset = (VRAMCNT_G.offset & 0x1) * 0x4000 + (VRAMCNT_G.offset & 0x2) * 0x8000;
    if (ADDR_IN_RANGE(VRAM_OBJA_START + g_offset, VRAM_G_SIZE) && VRAMCNT_G.MST == 2)
        reg |= *(T*)&VRAM_G[address & VRAM_G_MASK];
    //printf("\n(OBJA READ) $%08X: $%04X", address, reg);
    return reg;
}

template <typename T>
T GPU::read_objb(uint32_t address)
{
    T reg = 0;
    if (ADDR_IN_RANGE(VRAM_OBJB_START, VRAM_D_SIZE) && VRAMCNT_D.MST == 4)
    {
        if (VRAMCNT_D.enabled)
            reg |= *(T*)&VRAM_D[address & VRAM_D_MASK];
    }
    if (ADDR_IN_RANGE(VRAM_OBJB_START, VRAM_I_SIZE) && VRAMCNT_I.MST == 2)
    {
        if (VRAMCNT_I.enabled)
            reg |= *(T*)&VRAM_I[address & VRAM_I_MASK];
    }
    //printf("\n(OBJB READ) $%08X: $%04X", address, reg);
    return reg;
}

template <typename T>
T GPU::read_teximage(uint32_t address)
{
    T reg = 0;
    if (VRAMCNT_A.enabled)
    {
        uint32_t offset = VRAMCNT_A.offset * VRAM_A_SIZE;
        if (ADDR_IN_RANGE(offset, VRAM_A_SIZE) && VRAMCNT_A.MST == 3)
            reg |= *(T*)&VRAM_A[address & VRAM_A_MASK];
    }
    if (VRAMCNT_B.enabled)
    {
        uint32_t offset = VRAMCNT_B.offset * VRAM_B_SIZE;
        if (ADDR_IN_RANGE(offset, VRAM_B_SIZE) && VRAMCNT_B.MST == 3)
            reg |= *(T*)&VRAM_B[address & VRAM_B_MASK];
    }
    if (VRAMCNT_C.enabled)
    {
        uint32_t offset = VRAMCNT_C.offset * VRAM_C_SIZE;
        if (ADDR_IN_RANGE(offset, VRAM_C_SIZE) && VRAMCNT_C.MST == 3)
            reg |= *(T*)&VRAM_C[address & VRAM_C_MASK];
    }
    if (VRAMCNT_D.enabled)
    {
        uint32_t offset = VRAMCNT_D.offset * VRAM_D_SIZE;
        if (ADDR_IN_RANGE(offset, VRAM_D_SIZE) && VRAMCNT_D.MST == 3)
            reg |= *(T*)&VRAM_D[address & VRAM_D_MASK];
    }
    return reg;
}

template <typename T>
T GPU::read_texpal(uint32_t address)
{
    T reg = 0;
    bool lorp = false;
    if (VRAMCNT_E.enabled)
    {
        if (ADDR_IN_RANGE(0, VRAM_E_SIZE) && VRAMCNT_E.MST == 3)
        {
            reg |= *(T*)&VRAM_E[address & VRAM_E_MASK];
            lorp = true;
        }
    }
    if (VRAMCNT_F.enabled)
    {
        uint32_t addr = (VRAMCNT_F.offset & 0x1) + (VRAMCNT_F.offset & 0x2) * 2;
        addr *= VRAM_F_SIZE;
        if (ADDR_IN_RANGE(addr, VRAM_F_SIZE) && VRAMCNT_F.MST == 3)
        {
            reg |= *(T*)&VRAM_F[address & VRAM_F_MASK];
            lorp = true;
        }
    }
    if (VRAMCNT_G.enabled)
    {
        uint32_t addr = (VRAMCNT_G.offset & 0x1) + (VRAMCNT_G.offset & 0x2) * 2;
        addr *= VRAM_G_SIZE;
        if (ADDR_IN_RANGE(addr, VRAM_G_SIZE) && VRAMCNT_G.MST == 3)
        {
            reg |= *(T*)&VRAM_G[address & VRAM_G_MASK];
            lorp = true;
        }
    }
    return reg;
}

template <typename T>
T GPU::read_lcdc(uint32_t address)
{
    T reg = 0;
    if (VRAMCNT_A.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_A, VRAM_A_SIZE) && VRAMCNT_A.MST == 0)
            reg |= *(T*)&VRAM_A[address & VRAM_A_MASK];
    }
    if (VRAMCNT_B.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_B, VRAM_B_SIZE) && VRAMCNT_B.MST == 0)
            reg |= *(T*)&VRAM_B[address & VRAM_B_MASK];
    }
    if (VRAMCNT_C.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_C, VRAM_C_SIZE) && VRAMCNT_C.MST == 0)
            reg |= *(T*)&VRAM_C[address & VRAM_C_MASK];
    }
    if (VRAMCNT_D.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_D, VRAM_D_SIZE) && VRAMCNT_D.MST == 0)
            reg |= *(T*)&VRAM_D[address & VRAM_D_MASK];
    }
    if (VRAMCNT_E.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_E, VRAM_E_SIZE) && VRAMCNT_E.MST == 0)
            reg |= *(T*)&VRAM_E[address & VRAM_E_MASK];
    }
    if (VRAMCNT_F.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_F, VRAM_F_SIZE) && VRAMCNT_F.MST == 0)
            reg |= *(T*)&VRAM_F[address & VRAM_F_MASK];
    }
    if (VRAMCNT_G.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_G, VRAM_G_SIZE) && VRAMCNT_G.MST == 0)
            reg |= *(T*)&VRAM_G[address & VRAM_G_MASK];
    }
    if (VRAMCNT_H.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_H, VRAM_H_SIZE) && VRAMCNT_H.MST == 0)
            reg |= *(T*)&VRAM_H[address & VRAM_H_MASK];
    }
    if (VRAMCNT_I.enabled)
    {
        if (ADDR_IN_RANGE(VRAM_LCDC_I, VRAM_I_SIZE) && VRAMCNT_I.MST == 0)
            reg |= *(T*)&VRAM_I[address & VRAM_I_MASK];
    }
    return reg;
}

template <typename T>
T GPU::read_ARM7(uint32_t address)
{
    T reg = 0;
    if (VRAMCNT_C.enabled)
    {
        uint32_t offset = (VRAMCNT_C.offset & 0x1) * 0x20000;
        if (ADDR_IN_RANGE(0x06000000 + offset, VRAM_C_SIZE) && VRAMCNT_C.MST == 2)
            reg |= *(T*)&VRAM_C[address & VRAM_C_MASK];
    }
    if (VRAMCNT_D.enabled)
    {
        uint32_t offset = (VRAMCNT_D.offset & 0x1) * 0x20000;
        if (ADDR_IN_RANGE(0x06000000 + offset, VRAM_D_SIZE) && VRAMCNT_D.MST == 2)
            reg |= *(T*)&VRAM_D[address & VRAM_D_MASK];
    }
    return reg;
}

template <typename T>
T GPU::read_gba(uint32_t address)
{
    //I don't know if the NDS uses separate VRAM for GBA mode or if it re-uses VRAM banks
    //Might as well re-use the banks, since it makes no difference either way
    if (address < 0x06010000)
        return *(T*)&GBA_BG[address & 0xFFFF];
    else
        return *(T*)&GBA_OBJ[address & 0x7FFF];
}

template <typename T>
void GPU::write_ARM7(uint32_t address, T value)
{
    if (VRAMCNT_C.enabled)
    {
        uint32_t offset = (VRAMCNT_C.offset & 0x1) * 0x20000;
        if (ADDR_IN_RANGE(0x06000000 + offset, VRAM_C_SIZE) && VRAMCNT_C.MST == 2)
            *(T*)&VRAM_C[address & VRAM_C_MASK] = value;
    }
    if (VRAMCNT_D.enabled)
    {
        uint32_t offset = (VRAMCNT_D.offset & 0x1) * 0x20000;
        if (ADDR_IN_RANGE(0x06000000 + offset, VRAM_D_SIZE) && VRAMCNT_D.MST == 2)
            *(T*)&VRAM_D[address & VRAM_D_MASK] = value;
    }
}

template <typename T>
void GPU::write_gba(uint32_t address, T value)
{
    if (address < 0x06010000)
        *(T*)&GBA_BG[address & 0xFFFF] = value;
    else
        *(T*)&GBA_OBJ[address & 0x7FFF] = value;
}

template <typename T>
inline T GPU::read_OAM(uint32_t address)
{
    return *(T*)&OAM[address & 0x7FF];
}

inline void GPU::start_frame()
{
    frame_complete = false;
}

inline bool GPU::is_frame_complete()
{
    return frame_complete;
}

inline bool GPU::display_swapped()
{
    return POWCNT1.swap_display;
}

inline void GPU::set_upper_buffer(uint32_t* buffer)
{
    eng_A.set_framebuffer(buffer);
}

inline void GPU::set_lower_buffer(uint32_t *buffer)
{
    eng_B.set_framebuffer(buffer);
}

inline uint16_t GPU::get_VCOUNT()
{
    return VCOUNT;
}

#endif // GPU_HPP
