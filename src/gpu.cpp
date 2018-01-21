/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cmath>
#include <cstring>
#include "config.hpp"
#include "emulator.hpp"
#include "gpu.hpp"

GPU::GPU(Emulator* e) : e(e), eng_A(this, true), eng_B(this, false), eng_3D(e, this), frame_complete(false), cycles(0)
{
    eng_A.set_eng_3D(&eng_3D);
}

void GPU::power_on()
{
    eng_A.clear_buffer();
    eng_B.clear_buffer();
    eng_3D.power_on();
    cycles = 0;
    frame_complete = false;
    frames_skipped = 0;
    set_POWCNT1(0x820F);
    set_DISPCNT_A(0);
    set_DISPCNT_B(0);
    set_DISPSTAT7(0);
    set_DISPSTAT9(0);
    set_WIN0H_A(0);
    set_DISPCAPCNT(0);
    for (int i = 0; i < 4; i++)
    {
        set_BGHOFS_A(0, i);
        set_BGVOFS_A(0, i);
        set_BGHOFS_B(0, i);
        set_BGVOFS_B(0, i);
    }

    e->add_GPU_event(0, 256 * 6);

    memset(VRAM_A, 0, VRAM_A_SIZE);
    memset(VRAM_B, 0, VRAM_B_SIZE);
    memset(VRAM_C, 0, VRAM_C_SIZE);
    memset(VRAM_D, 0, VRAM_D_SIZE);
    memset(VRAM_E, 0, VRAM_E_SIZE);
    memset(VRAM_F, 0, VRAM_F_SIZE);
    memset(VRAM_G, 0, VRAM_G_SIZE);
    memset(VRAM_H, 0, VRAM_H_SIZE);
    memset(VRAM_I, 0, VRAM_I_SIZE);
}

void GPU::run_3D(uint64_t cycles)
{
    eng_3D.run(cycles);
}

void GPU::get_upper_frame(uint32_t* buffer)
{
    if (POWCNT1.swap_display)
        eng_A.get_framebuffer(buffer);
    else
        eng_B.get_framebuffer(buffer);
}

void GPU::get_lower_frame(uint32_t* buffer)
{
    if (POWCNT1.swap_display)
        eng_B.get_framebuffer(buffer);
    else
        eng_A.get_framebuffer(buffer);
}

void GPU::handle_event(SchedulerEvent &event)
{
    switch (event.id)
    {
        case 0: //Start HBLANK
            if (VCOUNT < SCANLINES && frames_skipped >= Config::frameskip)
                draw_scanline();
            //printf("\nStart HBLANK");
            DISPSTAT7.is_HBLANK = true;
            DISPSTAT9.is_HBLANK = true;

            if (DISPSTAT7.IRQ_on_HBLANK)
                e->request_interrupt7(INTERRUPT::HBLANK);
            if (DISPSTAT9.IRQ_on_HBLANK)
                e->request_interrupt9(INTERRUPT::HBLANK);
            if (VCOUNT < SCANLINES)
                e->HBLANK_DMA_request();
            e->add_GPU_event(1, 99 * 6);
            break;
        case 1: //End HBLANK
            //printf("\nEnd HBLANK");
            DISPSTAT7.is_HBLANK = false;
            DISPSTAT9.is_HBLANK = false;

            DISPSTAT7.is_VCOUNTER = (VCOUNT == DISPSTAT7.VCOUNTER);
            if (DISPSTAT7.is_VCOUNTER && DISPSTAT7.IRQ_on_VCOUNTER)
                e->request_interrupt7(INTERRUPT::VCOUNT_MATCH);

            DISPSTAT9.is_VCOUNTER = (VCOUNT == DISPSTAT9.VCOUNTER);
            if (DISPSTAT9.is_VCOUNTER && DISPSTAT9.IRQ_on_VCOUNTER)
                e->request_interrupt9(INTERRUPT::VCOUNT_MATCH);

            VCOUNT++;
            if (VCOUNT == SCANLINES)
            {
                //VBLANK
                //printf("\nVBLANK start");
                eng_3D.end_of_frame();
                frame_complete = true;
                if (DISPSTAT7.IRQ_on_VBLANK)
                    e->request_interrupt7(INTERRUPT::VBLANK);
                if (DISPSTAT9.IRQ_on_VBLANK)
                    e->request_interrupt9(INTERRUPT::VBLANK);
                DISPSTAT7.is_VBLANK = true;
                DISPSTAT9.is_VBLANK = true;

                eng_A.VBLANK_start();
                eng_B.VBLANK_start();
            }
            if (VCOUNT == 263)
            {
                VCOUNT = 0;
                DISPSTAT7.is_VBLANK = false;
                DISPSTAT9.is_VBLANK = false;
                if (frames_skipped >= Config::frameskip)
                    frames_skipped = 0;
                else
                    frames_skipped++;
            }
            e->add_GPU_event(0, 256 * 6);
            break;
    }
}

void GPU::draw_scanline()
{
    /*if (!POWCNT1.lcd_enable)
    {
        return;
    }*/
    if (POWCNT1.engine_a)
        eng_A.draw_scanline();
    if (POWCNT1.engine_b)
        eng_B.draw_scanline();
}

void GPU::check_GXFIFO_DMA()
{
    eng_3D.check_FIFO_DMA();
}

void GPU::check_GXFIFO_IRQ()
{
    eng_3D.check_FIFO_IRQ();
}

uint16_t GPU::read_palette_A(uint32_t address)
{
    return *(uint16_t*)&palette_A[address & 0x3FF];
}

uint16_t GPU::read_extpal_bga(uint32_t address)
{
    uint16_t reg = 0;
    if (VRAMCNT_E.enabled)
    {
        if (ADDR_IN_RANGE(0, VRAM_E_SIZE / 2) && VRAMCNT_E.MST == 4)
            reg |= *(uint16_t*)&VRAM_E[address & VRAM_E_MASK];
    }
    if (VRAMCNT_F.enabled)
    {
        int offset = (VRAMCNT_F.offset & 0x1) ? 1024 * 16 : 0;
        if (ADDR_IN_RANGE(offset, VRAM_F_SIZE) && VRAMCNT_F.MST == 4)
            reg |= *(uint16_t*)&VRAM_F[address & VRAM_F_MASK];
    }
    if (VRAMCNT_G.enabled)
    {
        int offset = (VRAMCNT_G.offset & 0x1) ? 1024 * 16 : 0;
        if (ADDR_IN_RANGE(offset, VRAM_G_SIZE) && VRAMCNT_G.MST == 4)
            reg |= *(uint16_t*)&VRAM_G[address & VRAM_G_MASK];
    }
    return reg;
}

uint16_t GPU::read_extpal_obja(uint32_t address)
{
    uint16_t reg = 0;
    if (VRAMCNT_F.enabled)
    {
        if (ADDR_IN_RANGE(0, VRAM_F_SIZE / 2) && VRAMCNT_F.MST == 5)
            reg |= *(uint16_t*)&VRAM_F[address & VRAM_F_MASK];
    }
    if (VRAMCNT_G.enabled)
    {
        if (ADDR_IN_RANGE(0, VRAM_G_SIZE / 2) && VRAMCNT_G.MST == 5)
            reg |= *(uint16_t*)&VRAM_G[address & VRAM_G_MASK];
    }
    return reg;
}

uint16_t GPU::read_palette_B(uint32_t address)
{
    return *(uint16_t*)&palette_B[address & 0x3FF];
}

uint16_t GPU::read_extpal_bgb(uint32_t address)
{
    uint16_t reg = 0;
    if (VRAMCNT_H.enabled)
    {
        if (ADDR_IN_RANGE(0, VRAM_H_SIZE) && VRAMCNT_H.MST == 2)
            reg |= *(uint16_t*)&VRAM_H[address & VRAM_H_MASK];
    }
    return reg;
}

uint16_t GPU::read_extpal_objb(uint32_t address)
{
    uint16_t reg = 0;
    if (VRAMCNT_I.enabled)
    {
        if (ADDR_IN_RANGE(0, VRAM_I_SIZE / 2) && VRAMCNT_I.MST == 3)
            reg |= *(uint16_t*)&VRAM_I[address & VRAM_I_MASK];
    }
    return reg;
}

void GPU::write_palette_A(uint32_t address, uint16_t halfword)
{
    *(uint16_t*)&palette_A[address & 0x3FF] = halfword;
}

void GPU::write_palette_B(uint32_t address, uint16_t halfword)
{
    *(uint16_t*)&palette_B[address & 0x3FF] = halfword;
}

void GPU::write_bga(uint32_t address, uint16_t halfword)
{
    //printf("\n(BGA) Write to $%08X of $%04X", address, halfword);
    if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_A.offset * 0x20000), VRAM_A_SIZE) && VRAMCNT_A.MST == 1)
    {
        if (VRAMCNT_A.enabled)
            *(uint16_t*)&VRAM_A[address & VRAM_A_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_B.offset * 0x20000), VRAM_B_SIZE) && VRAMCNT_B.MST == 1)
    {
        if (VRAMCNT_B.enabled)
            *(uint16_t*)&VRAM_B[address & VRAM_B_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_C.offset * 0x20000), VRAM_C_SIZE) && VRAMCNT_C.MST == 1)
    {
        if (VRAMCNT_C.enabled)
            *(uint16_t*)&VRAM_C[address & VRAM_C_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_BGA_START + (VRAMCNT_D.offset * 0x20000), VRAM_D_SIZE) && VRAMCNT_D.MST == 1)
    {
        if (VRAMCNT_D.enabled)
            *(uint16_t*)&VRAM_D[address & VRAM_D_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_BGA_START, VRAM_E_SIZE) && VRAMCNT_E.MST == 1)
    {
        if (VRAMCNT_E.enabled)
            *(uint16_t*)&VRAM_E[address & VRAM_E_MASK] = halfword;
    }
    uint32_t f_offset = (VRAMCNT_F.offset & 0x1) * 0x4000 + (VRAMCNT_F.offset & 0x2) * 0x8000;
    if (ADDR_IN_RANGE(VRAM_BGA_START + f_offset, VRAM_F_SIZE) && VRAMCNT_F.MST == 1)
    {
        if (VRAMCNT_F.enabled)
            *(uint16_t*)&VRAM_F[address & VRAM_F_MASK] = halfword;
    }
    uint32_t g_offset = (VRAMCNT_G.offset & 0x1) * 0x4000 + (VRAMCNT_G.offset & 0x2) * 0x8000;
    if (ADDR_IN_RANGE(VRAM_BGA_START + g_offset, VRAM_G_SIZE) && VRAMCNT_G.MST == 1)
    {
        if (VRAMCNT_G.enabled)
            *(uint16_t*)&VRAM_G[address & VRAM_G_MASK] = halfword;
    }
}

void GPU::write_bgb(uint32_t address, uint16_t halfword)
{
    //printf("\n(BGB) Write to $%08X: $%04X", address, halfword);
    if (ADDR_IN_RANGE(VRAM_BGB_C, VRAM_C_SIZE) && VRAMCNT_C.MST == 4)
    {
        if (VRAMCNT_C.enabled)
            *(uint16_t*)&VRAM_C[address & VRAM_C_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_BGB_H, VRAM_H_SIZE) && VRAMCNT_H.MST == 1)
    {
        if (VRAMCNT_H.enabled)
            *(uint16_t*)&VRAM_H[address & VRAM_H_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_BGB_I, VRAM_I_SIZE) && VRAMCNT_I.MST == 1)
    {
        if (VRAMCNT_I.enabled)
            *(uint16_t*)&VRAM_I[address & VRAM_I_MASK] = halfword;
    }
}

void GPU::write_obja(uint32_t address, uint16_t halfword)
{
    //printf("\n(OBJA WRITE) $%08X: $%04X", address, halfword);
    if (ADDR_IN_RANGE(VRAM_OBJA_START + ((VRAMCNT_A.offset & 0x1) * 0x20000), VRAM_A_SIZE) && VRAMCNT_A.MST == 2)
    {
        if (VRAMCNT_A.enabled)
            *(uint16_t*)&VRAM_A[address & VRAM_A_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_OBJA_START + ((VRAMCNT_B.offset & 0x1) * 0x20000), VRAM_B_SIZE) && VRAMCNT_B.MST == 2)
    {
        if (VRAMCNT_B.enabled)
            *(uint16_t*)&VRAM_B[address & VRAM_B_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_OBJA_START, VRAM_E_SIZE) && VRAMCNT_E.MST == 2)
    {
        if (VRAMCNT_E.enabled)
            *(uint16_t*)&VRAM_E[address & VRAM_E_MASK] = halfword;
    }
    uint32_t f_offset = (VRAMCNT_F.offset & 0x1) * 0x4000 + (VRAMCNT_F.offset & 0x2) * 0x8000;
    if (ADDR_IN_RANGE(VRAM_OBJA_START + f_offset, VRAM_F_SIZE) && VRAMCNT_F.MST == 2)
    {
        if (VRAMCNT_F.enabled)
            *(uint16_t*)&VRAM_F[address & VRAM_F_MASK] = halfword;
    }
    uint32_t g_offset = (VRAMCNT_G.offset & 0x1) * 0x4000 + (VRAMCNT_G.offset & 0x2) * 0x8000;
    if (ADDR_IN_RANGE(VRAM_OBJA_START + g_offset, VRAM_G_SIZE) && VRAMCNT_G.MST == 2)
    {
        if (VRAMCNT_G.enabled)
            *(uint16_t*)&VRAM_G[address & VRAM_G_MASK] = halfword;
    }
}

void GPU::write_objb(uint32_t address, uint16_t halfword)
{
    //printf("\n(OBJB WRITE) $%08X: $%04X", address, halfword);
    if (ADDR_IN_RANGE(VRAM_OBJB_START, VRAM_D_SIZE) && VRAMCNT_D.MST == 4)
    {
        if (VRAMCNT_D.enabled)
            *(uint16_t*)&VRAM_D[address & VRAM_D_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_OBJB_START, VRAM_I_SIZE) && VRAMCNT_I.MST == 2)
    {
        if (VRAMCNT_I.enabled)
            *(uint16_t*)&VRAM_I[address & VRAM_I_MASK] = halfword;
    }
}

void GPU::write_lcdc(uint32_t address, uint16_t halfword)
{
    if (ADDR_IN_RANGE(VRAM_LCDC_A, VRAM_A_SIZE) && VRAMCNT_A.MST == 0)
    {
        if (VRAMCNT_A.enabled)
            *(uint16_t*)&VRAM_A[address & VRAM_A_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_B, VRAM_B_SIZE) && VRAMCNT_B.MST == 0)
    {
        if (VRAMCNT_B.enabled)
            *(uint16_t*)&VRAM_B[address & VRAM_B_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_C, VRAM_C_SIZE) && VRAMCNT_C.MST == 0)
    {
        if (VRAMCNT_C.enabled)
            *(uint16_t*)&VRAM_C[address & VRAM_C_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_D, VRAM_D_SIZE) && VRAMCNT_D.MST == 0)
    {
        if (VRAMCNT_D.enabled)
            *(uint16_t*)&VRAM_D[address & VRAM_D_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_E, VRAM_E_SIZE) && VRAMCNT_E.MST == 0)
    {
        if (VRAMCNT_E.enabled)
            *(uint16_t*)&VRAM_E[address & VRAM_E_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_F, VRAM_F_SIZE) && VRAMCNT_F.MST == 0)
    {
        if (VRAMCNT_F.enabled)
            *(uint16_t*)&VRAM_F[address & VRAM_F_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_G, VRAM_G_SIZE) && VRAMCNT_G.MST == 0)
    {
        if (VRAMCNT_G.enabled)
            *(uint16_t*)&VRAM_G[address & VRAM_G_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_H, VRAM_H_SIZE) && VRAMCNT_H.MST == 0)
    {
        if (VRAMCNT_H.enabled)
            *(uint16_t*)&VRAM_H[address & VRAM_H_MASK] = halfword;
    }
    if (ADDR_IN_RANGE(VRAM_LCDC_I, VRAM_I_SIZE) && VRAMCNT_I.MST == 0)
    {
        if (VRAMCNT_I.enabled)
            *(uint16_t*)&VRAM_I[address & VRAM_I_MASK] = halfword;
    }
}

void GPU::write_OAM(uint32_t address, uint16_t halfword)
{
    *(uint16_t*)&OAM[address & 0x7FF] = halfword;
}

uint16_t* GPU::get_palette(bool engine_A)
{
    if (engine_A)
        return (uint16_t*)&palette_A;
    return (uint16_t*)&palette_B;
}

uint16_t* GPU::get_VRAM_block(int id)
{
    switch (id)
    {
        case 0:
            return (uint16_t*)&VRAM_A;
        case 1:
            return (uint16_t*)&VRAM_B;
        case 2:
            return (uint16_t*)&VRAM_C;
        case 3:
            return (uint16_t*)&VRAM_D;
        default:
            return nullptr;
    }
}

uint32_t GPU::get_DISPCNT_A()
{
    return eng_A.get_DISPCNT();
}

uint32_t GPU::get_DISPCNT_B()
{
    return eng_B.get_DISPCNT();
}

uint16_t GPU::get_DISPSTAT7()
{
    uint16_t reg = 0;
    reg |= DISPSTAT7.is_VBLANK;
    reg |= DISPSTAT7.is_HBLANK << 1;
    reg |= DISPSTAT7.is_VCOUNTER << 2;
    reg |= DISPSTAT7.IRQ_on_VBLANK << 3;
    reg |= DISPSTAT7.IRQ_on_HBLANK << 4;
    reg |= DISPSTAT7.IRQ_on_VCOUNTER << 5;
    reg |= (DISPSTAT7.VCOUNTER & 0x100) >> 1;
    reg |= (DISPSTAT7.VCOUNTER & ~0x100) << 8;
    return reg;
}

uint16_t GPU::get_DISPSTAT9()
{
    uint16_t reg = 0;
    reg |= DISPSTAT9.is_VBLANK;
    reg |= DISPSTAT9.is_HBLANK << 1;
    reg |= DISPSTAT9.is_VCOUNTER << 2;
    reg |= DISPSTAT9.IRQ_on_VBLANK << 3;
    reg |= DISPSTAT9.IRQ_on_HBLANK << 4;
    reg |= DISPSTAT9.IRQ_on_VCOUNTER << 5;
    reg |= (DISPSTAT9.VCOUNTER & 0x100) >> 1;
    reg |= (DISPSTAT9.VCOUNTER & ~0x100) << 8;
    return reg;
}

uint16_t GPU::get_BGCNT_A(int index)
{
    return eng_A.get_BGCNT(index);
}

uint16_t GPU::get_BGCNT_B(int index)
{
    return eng_B.get_BGCNT(index);
}

uint16_t GPU::get_BGHOFS_A(int index)
{
    return eng_A.get_BGHOFS(index);
}

uint16_t GPU::get_BGVOFS_A(int index)
{
    return eng_A.get_BGVOFS(index);
}

uint16_t GPU::get_BGHOFS_B(int index)
{
    return eng_B.get_BGHOFS(index);
}

uint16_t GPU::get_BGVOFS_B(int index)
{
    return eng_B.get_BGVOFS(index);
}

uint32_t GPU::get_BG2X_A()
{
    return eng_A.get_BG2X();
}

uint16_t GPU::get_WIN0V_A()
{
    return eng_A.get_WIN0V();
}

uint16_t GPU::get_WIN1V_A()
{
    return eng_A.get_WIN1V();
}

uint16_t GPU::get_WIN0V_B()
{
    return eng_B.get_WIN0V();
}

uint16_t GPU::get_WIN1V_B()
{
    return eng_B.get_WIN1V();
}

uint16_t GPU::get_WININ_A()
{
    return eng_A.get_WININ();
}

uint16_t GPU::get_WINOUT_A()
{
    return eng_A.get_WINOUT();
}

uint16_t GPU::get_WININ_B()
{
    return eng_B.get_WININ();
}

uint16_t GPU::get_WINOUT_B()
{
    return eng_B.get_WINOUT();
}

uint16_t GPU::get_BLDCNT_A()
{
    return eng_A.get_BLDCNT();
}

uint16_t GPU::get_BLDCNT_B()
{
    return eng_B.get_BLDCNT();
}

uint16_t GPU::get_BLDALPHA_A()
{
    return eng_A.get_BLDALPHA();
}

uint16_t GPU::get_BLDALPHA_B()
{
    return eng_B.get_BLDALPHA();
}

uint16_t GPU::get_DISP3DCNT()
{
    return eng_3D.get_DISP3DCNT();
}

uint16_t GPU::get_MASTER_BRIGHT_A()
{
    return eng_A.get_MASTER_BRIGHT();
}

uint16_t GPU::get_MASTER_BRIGHT_B()
{
    return eng_B.get_MASTER_BRIGHT();
}

uint32_t GPU::get_DISPCAPCNT()
{
    return eng_A.get_DISPCAPCNT();
}

//Tells the CPU if VRAM_C and/or VRAM_D are allocated for ARM7 usage
uint8_t GPU::get_VRAMSTAT()
{
    uint8_t reg = 0;
    reg |= (VRAMCNT_C.enabled && VRAMCNT_C.MST == 2);
    reg |= (VRAMCNT_D.enabled && VRAMCNT_D.MST == 2) << 1;
    return reg;
}

uint8_t GPU::get_VRAMCNT_A()
{
    uint8_t reg = 0;
    reg |= VRAMCNT_A.MST;
    reg |= VRAMCNT_A.offset << 3;
    reg |= VRAMCNT_A.enabled << 7;
    return reg;
}

uint8_t GPU::get_VRAMCNT_B()
{
    uint8_t reg = 0;
    reg |= VRAMCNT_B.MST;
    reg |= VRAMCNT_B.offset << 3;
    reg |= VRAMCNT_B.enabled << 7;
    return reg;
}

uint8_t GPU::get_VRAMCNT_C()
{
    uint8_t reg = 0;
    reg |= VRAMCNT_C.MST;
    reg |= VRAMCNT_C.offset << 3;
    reg |= VRAMCNT_C.enabled << 7;
    return reg;
}

uint8_t GPU::get_VRAMCNT_D()
{
    uint8_t reg = 0;
    reg |= VRAMCNT_D.MST;
    reg |= VRAMCNT_D.offset << 3;
    reg |= VRAMCNT_D.enabled << 7;
    return reg;
}

uint16_t GPU::get_POWCNT1()
{
    uint16_t reg = 0;
    reg |= POWCNT1.lcd_enable;
    reg |= POWCNT1.engine_a << 1;
    reg |= POWCNT1.rendering_3d << 2;
    reg |= POWCNT1.geometry_3d << 3;
    reg |= POWCNT1.engine_b << 9;
    reg |= POWCNT1.swap_display << 15;
    return reg;
}

uint32_t GPU::get_GXSTAT()
{
    return eng_3D.get_GXSTAT();
}

uint16_t GPU::get_vert_count()
{
    return eng_3D.get_vert_count();
}

uint16_t GPU::get_poly_count()
{
    return eng_3D.get_poly_count();
}

uint16_t GPU::read_vec_test(uint32_t address)
{
    return eng_3D.read_vec_test(address);
}

uint32_t GPU::read_clip_mtx(uint32_t address)
{
    return eng_3D.read_clip_mtx(address);
}

uint32_t GPU::read_vec_mtx(uint32_t address)
{
    return eng_3D.read_vec_mtx(address);
}

void GPU::set_DISPCNT_A_lo(uint16_t halfword)
{
    //TODO: handle forced blank bit 7
    eng_A.set_DISPCNT_lo(halfword);
}

void GPU::set_DISPCNT_A(uint32_t word)
{
    eng_A.set_DISPCNT(word);
    //printf("\nDISPCNT_A: $%08X", word);
}

void GPU::set_DISPCNT_B_lo(uint16_t halfword)
{
    eng_B.set_DISPCNT_lo(halfword);
}

void GPU::set_DISPCNT_B(uint32_t word)
{
    eng_B.set_DISPCNT(word);

    //printf("\nDISPCNT_B: $%08X", word);
}

void GPU::set_DISPSTAT7(uint16_t halfword)
{
    DISPSTAT7.IRQ_on_VBLANK = halfword & (1 << 3);
    DISPSTAT7.IRQ_on_HBLANK = halfword & (1 << 4);
    DISPSTAT7.IRQ_on_VCOUNTER = halfword & (1 << 5);
    DISPSTAT7.VCOUNTER = halfword >> 8;
    DISPSTAT7.VCOUNTER |= (halfword & (1 << 7)) << 1;
}

void GPU::set_DISPSTAT9(uint16_t halfword)
{
    DISPSTAT9.IRQ_on_VBLANK = halfword & (1 << 3);
    DISPSTAT9.IRQ_on_HBLANK = halfword & (1 << 4);
    DISPSTAT9.IRQ_on_VCOUNTER = halfword & (1 << 5);
    DISPSTAT9.VCOUNTER = halfword >> 8;
    DISPSTAT9.VCOUNTER |= (halfword & (1 << 7)) << 1;
}

void GPU::set_BGCNT_A(uint16_t halfword, int index)
{
    eng_A.set_BGCNT(halfword, index);
}

void GPU::set_BGCNT_B(uint16_t halfword, int index)
{
    eng_B.set_BGCNT(halfword, index);
}

void GPU::set_BGHOFS_A(uint16_t halfword, int index)
{
    eng_A.set_BGHOFS(halfword, index);
}

void GPU::set_BGVOFS_A(uint16_t halfword, int index)
{
    eng_A.set_BGVOFS(halfword, index);
}

void GPU::set_BGHOFS_B(uint16_t halfword, int index)
{
    eng_B.set_BGHOFS(halfword, index);
}

void GPU::set_BGVOFS_B(uint16_t halfword, int index)
{
    eng_B.set_BGVOFS(halfword, index);
}

void GPU::set_BG2P_A(uint16_t halfword, int index)
{
    eng_A.set_BG2P(halfword, index);
}

void GPU::set_BG2P_B(uint16_t halfword, int index)
{
    eng_B.set_BG2P(halfword, index);
}

void GPU::set_BG3P_A(uint16_t halfword, int index)
{
    eng_A.set_BG3P(halfword, index);
}

void GPU::set_BG3P_B(uint16_t halfword, int index)
{
    eng_B.set_BG3P(halfword, index);
}

void GPU::set_BG2X_A(uint32_t word)
{
    eng_A.set_BG2X(word);
}

void GPU::set_BG2Y_A(uint32_t word)
{
    eng_A.set_BG2Y(word);
}

void GPU::set_BG3X_A(uint32_t word)
{
    eng_A.set_BG3X(word);
}

void GPU::set_BG3Y_A(uint32_t word)
{
    eng_A.set_BG3Y(word);
}

void GPU::set_BG2X_B(uint32_t word)
{
    eng_B.set_BG2X(word);
}

void GPU::set_BG2Y_B(uint32_t word)
{
    eng_B.set_BG2Y(word);
}

void GPU::set_BG3X_B(uint32_t word)
{
    eng_B.set_BG3X(word);
}

void GPU::set_BG3Y_B(uint32_t word)
{
    eng_B.set_BG3Y(word);
}

void GPU::set_WIN0H_A(uint16_t halfword)
{
    eng_A.set_WIN0H(halfword);
}

void GPU::set_WIN1H_A(uint16_t halfword)
{
    eng_A.set_WIN1H(halfword);
}

void GPU::set_WIN0V_A(uint16_t halfword)
{
    eng_A.set_WIN0V(halfword);
}

void GPU::set_WIN1V_A(uint16_t halfword)
{
    eng_A.set_WIN1V(halfword);
}

void GPU::set_WIN0H_B(uint16_t halfword)
{
    eng_B.set_WIN0H(halfword);
}

void GPU::set_WIN1H_B(uint16_t halfword)
{
    eng_B.set_WIN1H(halfword);
}

void GPU::set_WIN0V_B(uint16_t halfword)
{
    eng_B.set_WIN0V(halfword);
}

void GPU::set_WIN1V_B(uint16_t halfword)
{
    eng_B.set_WIN1V(halfword);
}

void GPU::set_WININ_A(uint16_t halfword)
{
    eng_A.set_WININ(halfword);
}

void GPU::set_WININ_B(uint16_t halfword)
{
    eng_B.set_WININ(halfword);
}

void GPU::set_WINOUT_A(uint16_t halfword)
{
    eng_A.set_WINOUT(halfword);
}

void GPU::set_WINOUT_B(uint16_t halfword)
{
    eng_B.set_WINOUT(halfword);
}

void GPU::set_MOSAIC_A(uint16_t halfword)
{
    eng_A.set_MOSAIC(halfword);
}

void GPU::set_MOSAIC_B(uint16_t halfword)
{
    eng_B.set_MOSAIC(halfword);
}

void GPU::set_BLDCNT_A(uint16_t halfword)
{
    eng_A.set_BLDCNT(halfword);
}

void GPU::set_BLDCNT_B(uint16_t halfword)
{
    eng_B.set_BLDCNT(halfword);
}

void GPU::set_BLDALPHA_A(uint16_t halfword)
{
    eng_A.set_BLDALPHA(halfword);
}

void GPU::set_BLDALPHA_B(uint16_t halfword)
{
    eng_B.set_BLDALPHA(halfword);
}

void GPU::set_BLDY_A(uint8_t byte)
{
    eng_A.set_BLDY(byte);
}

void GPU::set_BLDY_B(uint8_t byte)
{
    eng_B.set_BLDY(byte);
}

void GPU::set_DISP3DCNT(uint16_t halfword)
{
    eng_3D.set_DISP3DCNT(halfword);
}

void GPU::set_MASTER_BRIGHT_A(uint16_t halfword)
{
    eng_A.set_MASTER_BRIGHT(halfword);
}

void GPU::set_MASTER_BRIGHT_B(uint16_t halfword)
{
    eng_B.set_MASTER_BRIGHT(halfword);
}

void GPU::set_DISPCAPCNT(uint32_t word)
{
    eng_A.set_DISPCAPCNT(word);
}

void GPU::set_VRAMCNT_A(uint8_t byte)
{
    VRAMCNT_A.MST = byte & 0x3;
    VRAMCNT_A.offset = (byte >> 3) & 0x3;
    VRAMCNT_A.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_B(uint8_t byte)
{
    VRAMCNT_B.MST = byte & 0x3;
    VRAMCNT_B.offset = (byte >> 3) & 0x3;
    VRAMCNT_B.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_C(uint8_t byte)
{
    VRAMCNT_C.MST = byte & 0x7;
    VRAMCNT_C.offset = (byte >> 3) & 0x3;
    VRAMCNT_C.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_D(uint8_t byte)
{
    VRAMCNT_D.MST = byte & 0x7;
    VRAMCNT_D.offset = (byte >> 3) & 0x3;
    VRAMCNT_D.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_E(uint8_t byte)
{
    VRAMCNT_E.MST = byte & 0x7;
    VRAMCNT_E.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_F(uint8_t byte)
{
    VRAMCNT_F.MST = byte & 0x7;
    VRAMCNT_F.offset = (byte >> 3) & 0x3;
    VRAMCNT_F.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_G(uint8_t byte)
{
    VRAMCNT_G.MST = byte & 0x7;
    VRAMCNT_G.offset = (byte >> 3) & 0x3;
    VRAMCNT_G.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_H(uint8_t byte)
{
    VRAMCNT_H.MST = byte & 0x3;
    VRAMCNT_H.enabled = byte & (1 << 7);
}

void GPU::set_VRAMCNT_I(uint8_t byte)
{
    VRAMCNT_I.MST = byte & 0x3;
    VRAMCNT_I.enabled = byte & (1 << 7);
}

void GPU::set_POWCNT1(uint16_t value)
{
    printf("\nPOWCNT1: $%04X", value);
    POWCNT1.lcd_enable = value & 0x1;
    POWCNT1.engine_a = value & (1 << 1);
    POWCNT1.rendering_3d = value & (1 << 2);
    POWCNT1.geometry_3d = value & (1 << 3);
    POWCNT1.engine_b = value & (1 << 9);
    POWCNT1.swap_display = value & (1 << 15);
}

void GPU::write_GXFIFO(uint32_t word)
{
    eng_3D.write_GXFIFO(word);
}

void GPU::write_FIFO_direct(uint32_t address, uint32_t word)
{
    eng_3D.write_FIFO_direct(address, word);
}

void GPU::set_CLEAR_COLOR(uint32_t word)
{
    eng_3D.set_CLEAR_COLOR(word);
}

void GPU::set_CLEAR_DEPTH(uint32_t word)
{
    eng_3D.set_CLEAR_DEPTH(word);
}

void GPU::set_EDGE_COLOR(uint32_t address, uint16_t halfword)
{
    eng_3D.set_EDGE_COLOR(address, halfword);
}

void GPU::set_FOG_COLOR(uint32_t word)
{
    eng_3D.set_FOG_COLOR(word);
}

void GPU::set_FOG_OFFSET(uint16_t halfword)
{
    eng_3D.set_FOG_OFFSET(halfword);
}

void GPU::set_FOG_TABLE(uint32_t address, uint8_t byte)
{
    eng_3D.set_FOG_TABLE(address, byte);
}

void GPU::set_MTX_MODE(uint32_t word)
{
    eng_3D.set_MTX_MODE(word);
}

void GPU::MTX_PUSH()
{
    eng_3D.MTX_PUSH();
}

void GPU::MTX_POP(uint32_t word)
{
    eng_3D.MTX_POP(word);
}

void GPU::MTX_IDENTITY()
{
    eng_3D.MTX_IDENTITY();
}

void GPU::MTX_MULT_4x4(uint32_t word)
{
    eng_3D.MTX_MULT_4x4(word);
}

void GPU::MTX_MULT_4x3(uint32_t word)
{
    eng_3D.MTX_MULT_4x3(word);
}

void GPU::MTX_MULT_3x3(uint32_t word)
{
    eng_3D.MTX_MULT_3x3(word);
}

void GPU::MTX_TRANS(uint32_t word)
{
    eng_3D.MTX_TRANS(word);
}

void GPU::COLOR(uint32_t word)
{
    eng_3D.COLOR(word);
}

void GPU::VTX_16(uint32_t word)
{
    //eng_3D.VTX_16(word);
}

void GPU::set_POLYGON_ATTR(uint32_t word)
{
    eng_3D.set_POLYGON_ATTR(word);
}

void GPU::set_TEXIMAGE_PARAM(uint32_t word)
{
    eng_3D.set_TEXIMAGE_PARAM(word);
}

void GPU::set_TOON_TABLE(uint32_t address, uint16_t color)
{
    eng_3D.set_TOON_TABLE(address, color);
}

void GPU::BEGIN_VTXS(uint32_t word)
{
    eng_3D.BEGIN_VTXS(word);
}

void GPU::SWAP_BUFFERS(uint32_t word)
{
    eng_3D.write_FIFO_direct(0x4000540, word);
    //eng_3D.SWAP_BUFFERS(word);
}

void GPU::VIEWPORT(uint32_t word)
{
    eng_3D.VIEWPORT(word);
}

void GPU::set_GXSTAT(uint32_t word)
{
    eng_3D.set_GXSTAT(word);
}
