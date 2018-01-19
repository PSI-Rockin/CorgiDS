/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef memconsts_h
#define memconsts_h

#define ADDR_IN_RANGE(x, s) address >= x && address < x + s
#define MASKED_ADDR(x, s, m) address >= x && ((address + x) & m) < s

//Memory addresses
#define MAIN_RAM_START          0x02000000
#define SHARED_WRAM_START       0x03000000
#define ARM7_WRAM_START         0x03800000
#define IO_REGS_START           0x04000000
#define PALETTE_START           0x05000000
#define VRAM_BGA_START          0x06000000
#define VRAM_BGB_START          0x06200000
#define VRAM_BGB_C              0x06200000
#define VRAM_BGB_H              0x06200000
#define VRAM_BGB_I              0x06208000
#define VRAM_OBJA_START         0x06400000
#define VRAM_OBJB_START         0x06600000
#define VRAM_LCDC_A             0x06800000
#define VRAM_LCDC_B             0x06820000
#define VRAM_LCDC_C             0x06840000
#define VRAM_LCDC_D             0x06860000
#define VRAM_LCDC_E             0x06880000
#define VRAM_LCDC_F             0x06890000
#define VRAM_LCDC_G             0x06894000
#define VRAM_LCDC_H             0x06898000
#define VRAM_LCDC_I             0x068A0000
#define VRAM_LCDC_END           0x068A4000
#define OAM_START               0x07000000
#define GBA_ROM_START           0x08000000
#define GBA_RAM_START           0x0A000000

//Sizes
#define VRAM_A_SIZE             1024 * 128
#define VRAM_B_SIZE             1024 * 128
#define VRAM_C_SIZE             1024 * 128
#define VRAM_D_SIZE             1024 * 128
#define VRAM_E_SIZE             1024 * 64
#define VRAM_F_SIZE             1024 * 16
#define VRAM_G_SIZE             1024 * 16
#define VRAM_H_SIZE             1024 * 32
#define VRAM_I_SIZE             1024 * 16
#define BIOS9_SIZE              1024 * 4
#define BIOS7_SIZE              1024 * 16
#define BIOS_GBA_SIZE           1024 * 16

//Masks
#define ITCM_MASK               0x7FFF
#define DTCM_MASK               0x3FFF
#define ARM7_WRAM_MASK          0xFFFF
#define MAIN_RAM_MASK           0x3FFFFF
#define VRAM_A_MASK             (1024 * 128) - 1
#define VRAM_B_MASK             (1024 * 128) - 1
#define VRAM_C_MASK             (1024 * 128) - 1
#define VRAM_D_MASK             (1024 * 128) - 1
#define VRAM_E_MASK             (1024 * 64) - 1
#define VRAM_F_MASK             (1024 * 16) - 1
#define VRAM_G_MASK             (1024 * 16) - 1
#define VRAM_H_MASK             (1024 * 32) - 1
#define VRAM_I_MASK             (1024 * 16) - 1

//Other constants
#define PIXELS_PER_LINE         256
#define GBA_PIXELS_PER_LINE     240
#define SCANLINES               192
#define GBA_SCANLINES           160

#endif
