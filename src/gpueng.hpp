/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef GPUENG_HPP
#define GPUENG_HPP
#include <cstdint>
#include "memconsts.h"

struct DISPCNT_REG
{
    int bg_mode;
    bool bg_3d;
    bool tile_obj_1d;
    bool bitmap_obj_square;
    bool bitmap_obj_1d;
    bool display_bg0;
    bool display_bg1;
    bool display_bg2;
    bool display_bg3;
    bool display_obj;
    bool display_win0;
    bool display_win1;
    bool obj_win_display;
    int display_mode;
    int VRAM_block;
    int tile_obj_1d_bound;
    bool bitmap_obj_1d_bound;
    bool hblank_obj_processing;
    int char_base;
    int screen_base;
    bool bg_extended_palette;
    bool obj_extended_palette;
};

struct DISPCAPCNT_REG
{
    int EVA;
    int EVB;
    int VRAM_write_block;
    int VRAM_write_offset;
    int capture_size;
    bool A_3D_only;
    bool B_display_FIFO;
    int VRAM_read_offset;
    int capture_source;
    bool enable_busy;
};

struct WININ_REG
{
    bool win0_bg_enabled[4];
    bool win0_obj_enabled;
    bool win0_color_special;
    bool win1_bg_enabled[4];
    bool win1_obj_enabled;
    bool win1_color_special;
};

struct WINOUT_REG
{
    bool outside_bg_enabled[4];
    bool outside_obj_enabled;
    bool outside_color_special;
    bool objwin_bg_enabled[4];
    bool objwin_obj_enabled;
    bool objwin_color_special;
};

struct BLDCNT_REG
{
    bool bg_first_target_pix[4];
    bool obj_first_target_pix;
    bool bd_first_target_pix;
    int effect;
    bool bg_second_target_pix[4];
    bool obj_second_target_pix;
    bool bd_second_target_pix;
};

class GPU;
class GPU_3D;

class GPU_2D_Engine
{
    private:
        GPU* gpu;
        GPU_3D* eng_3D;
        uint32_t framebuffer[PIXELS_PER_LINE * SCANLINES], front_framebuffer[PIXELS_PER_LINE * SCANLINES];
        uint32_t second_layer[PIXELS_PER_LINE];
        uint8_t layer_sources[PIXELS_PER_LINE * 2];
        uint8_t final_bg_priority[PIXELS_PER_LINE * 2];
        uint32_t sprite_scanline[PIXELS_PER_LINE * 2];
        uint8_t window_mask[PIXELS_PER_LINE];
        bool engine_A;

        DISPCNT_REG DISPCNT;
        DISPCAPCNT_REG DISPCAPCNT;
        int captured_lines;

        uint16_t BGCNT[4];
        uint16_t BGHOFS[4];
        uint16_t BGVOFS[4];

        uint16_t BG2P[4];
        uint16_t BG3P[4];
        uint32_t BG2X, BG2Y;
        uint32_t BG3X, BG3Y;
        int16_t BG2P_internal[4];
        int16_t BG3P_internal[4];
        int32_t BG2X_internal, BG2Y_internal;
        int32_t BG3X_internal, BG3Y_internal;

        uint16_t WIN0H, WIN1H;
        uint16_t WIN0V, WIN1V;

        uint16_t MOSAIC;

        WININ_REG WININ;
        WINOUT_REG WINOUT;
        bool win0_active, win1_active;

        BLDCNT_REG BLDCNT;
        uint16_t BLDALPHA;
        uint8_t BLDY;

        uint16_t MASTER_BRIGHT;

        void draw_3D();
        void draw_ext_text(int index);
        void draw_pixel(int x, int y, uint16_t color, int source);
        void get_window_mask();
        void handle_BLDCNT_effects();

        void gba_draw_txt(int index);
        void gba_draw_mode2(int index);
        void gba_draw_sprites();
        void gba_draw_pixel(int x, int y, uint16_t color, int source);
        void gba_handle_BLDCNT();
    public:
        GPU_2D_Engine(GPU* gpu, bool engine_A);
        void draw_backdrop();
        void draw_bg_txt(int index);
        void draw_bg_aff(int index);
        void draw_bg_ext(int index);
        void draw_sprites(bool objwin);
        void draw_rotscale_sprite(uint16_t* attributes, bool objwin);
        void draw_scanline();

        void get_framebuffer(uint32_t* buffer);
        void set_framebuffer(uint32_t* buffer);
        void clear_buffer();

        void VBLANK_start();

        uint32_t get_DISPCNT();
        uint16_t get_BGCNT(int index);

        uint16_t get_BGHOFS(int index);
        uint16_t get_BGVOFS(int index);

        uint32_t get_BG2X();

        uint16_t get_WIN0V();
        uint16_t get_WIN1V();

        uint16_t get_WININ();
        uint16_t get_WINOUT();

        uint16_t get_BLDCNT();
        uint16_t get_BLDALPHA();

        uint16_t get_MASTER_BRIGHT();
        uint32_t get_DISPCAPCNT();

        void set_eng_3D(GPU_3D* eng_3D);

        void set_DISPCNT_lo(uint16_t halfword);
        void set_DISPCNT(uint32_t word);

        void set_BGCNT(uint16_t halfword, int index);
        void set_BGHOFS(uint16_t halfword, int index);
        void set_BGVOFS(uint16_t halfword, int index);

        void set_BG2P(uint16_t halfword, int index);
        void set_BG3P(uint16_t halfword, int index);

        void set_BG2X(uint32_t word);
        void set_BG2Y(uint32_t word);
        void set_BG3X(uint32_t word);
        void set_BG3Y(uint32_t word);

        void set_WIN0H(uint16_t halfword);
        void set_WIN1H(uint16_t halfword);
        void set_WIN0V(uint16_t halfword);
        void set_WIN1V(uint16_t halfword);

        void set_MOSAIC(uint16_t halfword);

        void set_WININ(uint16_t halfword);
        void set_WINOUT(uint16_t halfword);

        void set_BLDCNT(uint16_t halfword);
        void set_BLDALPHA(uint16_t halfword);
        void set_BLDY(uint8_t byte);

        void set_MASTER_BRIGHT(uint16_t halfword);
        void set_DISPCAPCNT(uint32_t word);

        void gba_draw_scanline();
        void gba_set_DISPCNT(uint16_t halfword);
};

#endif // GPUENG_HPP
