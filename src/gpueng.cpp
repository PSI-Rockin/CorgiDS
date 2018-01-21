/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "config.hpp"
#include "gpu.hpp"
#include "gpueng.hpp"

GPU_2D_Engine::GPU_2D_Engine(GPU* gpu, bool engine_A) : gpu(gpu), eng_3D(nullptr), engine_A(engine_A) {}

void GPU_2D_Engine::VBLANK_start()
{
    for (int i = 0; i < 4; i++)
    {
        BG2P_internal[i] = BG2P[i];
        BG3P_internal[i] = BG3P[i];
    }

    BG2X_internal = BG2X;
    BG2Y_internal = BG2Y;
    BG3X_internal = BG3X;
    BG3Y_internal = BG3Y;

    DISPCAPCNT.enable_busy = false;
}

void GPU_2D_Engine::draw_backdrop()
{
    uint16_t* palette = gpu->get_palette(engine_A);
    for (int x = 0; x < PIXELS_PER_LINE; x++)
        draw_pixel(x, gpu->get_VCOUNT(), palette[0], 5);
}

void GPU_2D_Engine::draw_pixel(int x, int y, uint16_t color, int source)
{
    int pixel = x + (y * PIXELS_PER_LINE);
    uint8_t r = (color & 0x1F) << 1;
    uint8_t g = ((color >> 5) & 0x1F) << 1;
    uint8_t b = ((color >> 10) & 0x1F) << 1;
    uint32_t new_color = 0xFF000000 | (r << 16) | (g << 8) | b;
    layer_sources[x + 256] = layer_sources[x];
    second_layer[x] = framebuffer[pixel];
    layer_sources[x] = (1 << source);
    framebuffer[pixel] = new_color;
}

void GPU_2D_Engine::get_window_mask()
{
    //Determine if the windows are active on this scanline
    //Note: only the lower 8 bits of VCOUNT are used
    int line = gpu->get_VCOUNT() & 0xFF;

    int y1_0 = WIN0V >> 8, y2_0 = WIN0V & 0xFF;
    int y1_1 = WIN1V >> 8, y2_1 = WIN1V & 0xFF;
    if (line == y1_0)
        win0_active = true;
    else if (line == y2_0 || (!line && y2_0 == 0xC0))
        win0_active = false;

    if (line == y1_1)
        win1_active = true;
    else if (line == y2_1 || (!line && y2_1 == 0xC0))
        win1_active = false;

    //Reset window mask to outside window
    for (int i = 0; i < PIXELS_PER_LINE; i++)
        window_mask[i] = get_WINOUT() & 0xFF;

    if (DISPCNT.obj_win_display && DISPCNT.display_obj)
    {
        draw_sprites(true);
        for (int x = 0; x < PIXELS_PER_LINE; x++)
        {
            if ((sprite_scanline[x] & 0x90000000) == 0x90000000)
                window_mask[x] = get_WINOUT() >> 8;
        }
    }

    if (DISPCNT.display_win1 && win1_active)
    {
        int x1 = WIN1H >> 8, x2 = WIN1H & 0xFF;
        for (int x = x1; x < x2; x++)
            window_mask[x] = get_WININ() >> 8;
    }

    if (DISPCNT.display_win0 && win0_active)
    {
        int x1 = WIN0H >> 8, x2 = WIN0H & 0xFF;
        for (int x = x1; x < x2; x++)
            window_mask[x] = get_WININ() & 0xFF;
    }
}

void GPU_2D_Engine::draw_bg_txt(int index)
{
    uint16_t x_offset = BGHOFS[index];
    uint16_t y_offset = BGVOFS[index] + gpu->get_VCOUNT();
    uint16_t* palette = gpu->get_palette(engine_A);

    bool one_palette_mode = BGCNT[index] & (1 << 7);

    int screen_base, char_base;
    if (engine_A)
    {
        screen_base = VRAM_BGA_START + (DISPCNT.screen_base * 1024 * 64);
        char_base = VRAM_BGA_START + (DISPCNT.char_base * 1024 * 64);
    }
    else
    {
        screen_base = VRAM_BGB_C;
        char_base = VRAM_BGB_C;
    }

    screen_base += ((BGCNT[index] >> 8) & 0x1F) * 1024 * 2;

    if (BGCNT[index] & 0x8000)
    {
        screen_base += (y_offset & 0x1F8) * 8;
        if (BGCNT[index] & 0x4000)
            screen_base += (y_offset & 0x100) * 8;
    }
    else
        screen_base += (y_offset & 0xF8) * 8;
    char_base += ((BGCNT[index] >> 2) & 0xF) * 1024 * 16;

    uint16_t tile;
    int extpal_base = index * 1024 * 8;
    int tile_num;
    int palette_id;
    int pixel_base;
    bool y_flip;
    bool x_flip;
    uint16_t color;

    int wide_x = (BGCNT[index] & (1 << 14)) ? 0x100 : 0;

    if (!one_palette_mode)
    {
        uint32_t data;
        if (x_offset & 0x7)
        {
            if (engine_A)
                tile = gpu->read_bga<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));
            else
                tile = gpu->read_bgb<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));

            tile_num = tile & 0x3FF;
            x_flip = tile & (1 << 10);
            y_flip = tile & (1 << 11);
            palette_id = tile >> 12;

            pixel_base = char_base + (tile_num * 32) + (y_offset & 0x7) * 4;

            if (engine_A)
                data = gpu->read_bga<uint32_t>(pixel_base);
            else
                data = gpu->read_bgb<uint32_t>(pixel_base);
        }
        for (int pixel = 0; pixel < PIXELS_PER_LINE; pixel++)
        {
            if (!(x_offset & 0x7))
            {
                if (engine_A)
                    tile = gpu->read_bga<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));
                else
                    tile = gpu->read_bgb<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));

                tile_num = tile & 0x3FF;
                x_flip = tile & (1 << 10);
                y_flip = tile & (1 << 11);
                palette_id = tile >> 12;
                pixel_base = char_base + (tile_num * 32) + ((y_flip) ? (7 - (y_offset & 0x7)) * 4 : (y_offset & 0x7) * 4);

                if (engine_A)
                    data = gpu->read_bga<uint32_t>(pixel_base);
                else
                    data = gpu->read_bgb<uint32_t>(pixel_base);

            }
            int tile_x_offset = (x_flip) ? 7 - (x_offset & 0x7): x_offset & 0x7;
            color = (data >> (tile_x_offset * 4)) & 0xF;

            //Draw the pixel
            //Palette color 0 is transparent, so skip drawing that
            if (color && (window_mask[pixel] & (1 << index)))
            {
                color = palette[(palette_id * 16) + color];
                draw_pixel(pixel, gpu->get_VCOUNT(), color, index);
                final_bg_priority[pixel] = BGCNT[index] & 0x3;
            }
            x_offset++;
        }
    }
    else
    {
        uint64_t data;
        if (x_offset & 0x7)
        {
            if (engine_A)
                tile = gpu->read_bga<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));
            else
                tile = gpu->read_bgb<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));

            tile_num = tile & 0x3FF;
            x_flip = tile & (1 << 10);
            y_flip = tile & (1 << 11);
            palette_id = tile >> 12;

            pixel_base = char_base + (tile_num * 64);
            pixel_base += ((y_flip) ? (7 - (y_offset & 0x7)) * 8 : (y_offset & 0x7) * 8);

            if (engine_A)
                data = gpu->read_bga<uint64_t>(pixel_base);
            else
                data = gpu->read_bgb<uint64_t>(pixel_base);
        }
        for (int pixel = 0; pixel < PIXELS_PER_LINE; pixel++)
        {
            if (!(x_offset & 0x7))
            {
                if (engine_A)
                    tile = gpu->read_bga<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));
                else
                    tile = gpu->read_bgb<uint16_t>(screen_base + ((x_offset & 0xF8) >> 2) + ((x_offset & wide_x) << 3));

                tile_num = tile & 0x3FF;
                x_flip = tile & (1 << 10);
                y_flip = tile & (1 << 11);
                palette_id = tile >> 12;

                pixel_base = char_base + (tile_num * 64);
                pixel_base += ((y_flip) ? (7 - (y_offset & 0x7)) * 8 : (y_offset & 0x7) * 8);

                if (engine_A)
                    data = gpu->read_bga<uint64_t>(pixel_base);
                else
                    data = gpu->read_bgb<uint64_t>(pixel_base);
            }

            int tile_x_offset = (x_flip) ? 7 - (x_offset & 0x7): x_offset & 0x7;
            color = (data >> (tile_x_offset * 8)) & 0xFF;
            if (color && (window_mask[pixel] & (1 << index)))
            {
                if (DISPCNT.bg_extended_palette)
                {
                    if (engine_A)
                        color = gpu->read_extpal_bga(extpal_base + (palette_id * 512) + (color * 2));
                    else
                        color = gpu->read_extpal_bgb(extpal_base + (palette_id * 512) + (color * 2));
                }
                else
                    color = palette[color];

                draw_pixel(pixel, gpu->get_VCOUNT(), color, index);
                final_bg_priority[pixel] = BGCNT[index] & 0x3;
            }
            x_offset++;
        }
    }
}

void GPU_2D_Engine::draw_bg_aff(int index)
{
    int16_t rot_A, rot_B, rot_C, rot_D;
    int32_t x_offset, y_offset;
    if (index == 2)
    {
        rot_A = (int16_t)BG2P_internal[0];
        rot_B = (int16_t)BG2P_internal[1];
        rot_C = (int16_t)BG2P_internal[2];
        rot_D = (int16_t)BG2P_internal[3];

        x_offset = (int32_t)BG2X_internal;
        y_offset = (int32_t)BG2Y_internal;
    }
    else
    {
        rot_A = (int16_t)BG3P_internal[0];
        rot_B = (int16_t)BG3P_internal[1];
        rot_C = (int16_t)BG3P_internal[2];
        rot_D = (int16_t)BG3P_internal[3];

        x_offset = (int32_t)BG3X_internal;
        y_offset = (int32_t)BG3Y_internal;
    }

    uint32_t screen_base;
    uint32_t char_base;
    if (engine_A)
    {
        screen_base = VRAM_BGA_START + (DISPCNT.screen_base * 1024 * 64);
        char_base = VRAM_BGA_START + (DISPCNT.char_base * 1024 * 64);
    }
    else
    {
        screen_base = VRAM_BGB_START;
        char_base = VRAM_BGB_START;
    }

    screen_base += ((BGCNT[index] >> 8) & 0x1F) * 1024 * 2;
    char_base += ((BGCNT[index] >> 2) & 0xF) * 1024 * 16;

    int screen_size = BGCNT[index] >> 14;
    uint32_t mask;
    switch (screen_size)
    {
        case 0:
            mask = 0x7800;
            break;
        case 1:
            mask = 0xF800;
            break;
        case 2:
            mask = 0x1F800;
            break;
        case 3:
            mask = 0x3F800;
            break;
    }
    int y_factor = screen_size + 4;
    uint32_t overflow_mask = (BGCNT[index] & (1 << 13)) ? 0 : ~(mask | 0x7FF);
    uint16_t* palette = gpu->get_palette(engine_A);
    for (int pixel = 0; pixel < PIXELS_PER_LINE; pixel++)
    {
        if (window_mask[pixel] & (1 << index))
        {
            if ((!((x_offset | y_offset) & overflow_mask)))
            {
                uint16_t tile;
                uint16_t color;
                uint32_t tile_addr = ((((y_offset & mask) >> 11) << y_factor) + ((x_offset & mask) >> 11));

                uint32_t tile_x_offset = (x_offset >> 8) & 0x7;
                uint32_t tile_y_offset = (y_offset >> 8) & 0x7;
                if (engine_A)
                    tile = gpu->read_bga<uint8_t>(screen_base + tile_addr);
                else
                    tile = gpu->read_bgb<uint8_t>(screen_base + tile_addr);

                uint32_t char_addr = (tile << 6) + (tile_y_offset << 3) + tile_x_offset;
                if (engine_A)
                    color = gpu->read_bga<uint8_t>(char_base + char_addr);
                else
                    color = gpu->read_bgb<uint8_t>(char_base + char_addr);

                if (color)
                {
                    color = palette[color];
                    draw_pixel(pixel, gpu->get_VCOUNT(), color, index);
                    final_bg_priority[pixel] = BGCNT[index] & 0x3;
                }
            }
        }
        x_offset += rot_A;
        y_offset += rot_C;
    }

    if (index == 2)
    {
        BG2X_internal += rot_B;
        BG2Y_internal += rot_D;
    }
    else
    {
        BG3X_internal += rot_B;
        BG3Y_internal += rot_D;
    }
}

void GPU_2D_Engine::draw_bg_ext(int index)
{
    uint32_t base;
    if (engine_A)
        base = VRAM_BGA_START;
    else
        base = VRAM_BGB_START;
    int y_offset = gpu->get_VCOUNT();
    if (index == 2)
        y_offset += BG2Y >> 8;
    else
        y_offset += BG3Y >> 8;
    base += ((BGCNT[index] >> 8) & 0x1F) * 1024 * 16;
    int bg_mode = (BGCNT[index] & (1 << 2)) != 0;
    bg_mode += ((BGCNT[index] & (1 << 7)) != 0) << 1;

    //TODO: apply rotscale to modes 0-2
    switch (bg_mode)
    {
        case 0:
        case 1:
            draw_ext_text(index);
            break;
        //Rotscale 256-color bitmap
        case 2:
            for (int i = 0; i < PIXELS_PER_LINE; i++)
            {
                int color;
                if (engine_A)
                {
                    color = gpu->read_bga<uint8_t>(base + i + (gpu->get_VCOUNT() * PIXELS_PER_LINE));
                    color = gpu->read_palette_A(color * 2);
                }
                else
                {
                    color = gpu->read_bgb<uint8_t>(base + i + (gpu->get_VCOUNT() * PIXELS_PER_LINE));
                    color = gpu->read_palette_B(color * 2);
                }

                if (color)
                {
                    draw_pixel(i, gpu->get_VCOUNT(), color, index);
                    final_bg_priority[i] = BGCNT[index] & 0x3;
                }
            }
            break;
        //Direct color bitmap
        case 3:
            for (int i = 0; i < PIXELS_PER_LINE; i++)
            {
                uint16_t ds_color;
                if (engine_A)
                    ds_color = gpu->read_bga<uint16_t>(base + (i * 2) + (y_offset * PIXELS_PER_LINE * 2));
                else
                    ds_color = gpu->read_bgb<uint16_t>(base + (i * 2) + (y_offset * PIXELS_PER_LINE * 2));
                if (!(ds_color & (1 << 15)))
                    continue;
                draw_pixel(i, gpu->get_VCOUNT(), ds_color, index);
                final_bg_priority[i] = BGCNT[index] & 0x3;
            }
            break;
        default:
            printf("\nUnrecognized extended mode %d", bg_mode);
    }
}

void GPU_2D_Engine::draw_ext_text(int index)
{
    int16_t rot_A, rot_B, rot_C, rot_D;
    int32_t x_offset, y_offset;
    if (index == 2)
    {
        rot_A = (int16_t)BG2P_internal[0];
        rot_B = (int16_t)BG2P_internal[1];
        rot_C = (int16_t)BG2P_internal[2];
        rot_D = (int16_t)BG2P_internal[3];

        x_offset = (int32_t)BG2X_internal;
        y_offset = (int32_t)BG2Y_internal;
    }
    else
    {
        rot_A = (int16_t)BG3P_internal[0];
        rot_B = (int16_t)BG3P_internal[1];
        rot_C = (int16_t)BG3P_internal[2];
        rot_D = (int16_t)BG3P_internal[3];

        x_offset = (int32_t)BG3X_internal;
        y_offset = (int32_t)BG3Y_internal;
    }
    uint32_t screen_base;
    uint32_t char_base;
    if (engine_A)
    {
        screen_base = VRAM_BGA_START + (DISPCNT.screen_base * 1024 * 64);
        char_base = VRAM_BGA_START + (DISPCNT.char_base * 1024 * 64);
    }
    else
    {
        screen_base = VRAM_BGB_C;
        char_base = VRAM_BGB_C;
    }

    screen_base += ((BGCNT[index] >> 8) & 0x1F) * 1024 * 2;
    char_base += ((BGCNT[index] >> 2) & 0xF) * 1024 * 16;

    int screen_size = BGCNT[index] >> 14;
    uint32_t mask;
    switch (screen_size)
    {
        case 0:
            mask = 0x7800;
            break;
        case 1:
            mask = 0xF800;
            break;
        case 2:
            mask = 0x1F800;
            break;
        case 3:
            mask = 0x3F800;
            break;
    }
    int y_factor = screen_size + 4;

    int extpal_base = index * 1024 * 8;
    uint32_t overflow_mask = (BGCNT[index] & (1 << 13)) ? 0 : ~(mask | 0x7FF);
    for (int pixel = 0; pixel < PIXELS_PER_LINE; pixel++)
    {
        if (!((x_offset | y_offset) & overflow_mask))
        {
            uint16_t tile;
            uint32_t tile_addr_offset = ((y_offset & mask) >> 11) << y_factor;
            tile_addr_offset += (x_offset & mask) >> 11;
            tile_addr_offset <<= 1;
            if (engine_A)
                tile = gpu->read_bga<uint16_t>(screen_base + tile_addr_offset);
            else
                tile = gpu->read_bgb<uint16_t>(screen_base + tile_addr_offset);

            int char_id = tile & 0x3FF;
            bool x_flip = tile & (1 << 10);
            bool y_flip = tile & (1 << 11);
            int palette_id = tile >> 12;

            int tile_x_offset = (x_offset >> 8) & 0x7;
            int tile_y_offset = (y_offset >> 8) & 0x7;

            if (x_flip)
                tile_x_offset = 7 - tile_x_offset;
            if (y_flip)
                tile_y_offset = 7 - tile_y_offset;

            int color;
            if (engine_A)
                color = gpu->read_bga<uint8_t>(char_base + (char_id << 6) + (tile_y_offset << 3) + tile_x_offset);
            else
                color = gpu->read_bgb<uint8_t>(char_base + (char_id << 6) + (tile_y_offset << 3) + tile_x_offset);

            if (color)
            {
                if (DISPCNT.bg_extended_palette)
                {
                    if (engine_A)
                        color = gpu->read_extpal_bga(extpal_base + color * 2 + palette_id * 512);
                    else
                        color = gpu->read_extpal_bgb(extpal_base + color * 2 + palette_id * 512);
                }
                else
                {
                    if (engine_A)
                        color = gpu->read_palette_A(color * 2);
                    else
                        color = gpu->read_palette_B(color * 2);
                }

                draw_pixel(pixel, gpu->get_VCOUNT(), color, index);
                final_bg_priority[pixel] = BGCNT[index] & 0x3;
            }
        }
        x_offset += rot_A;
        y_offset += rot_C;
    }

    if (index == 2)
    {
        BG2X_internal += rot_B;
        BG2Y_internal += rot_D;
    }
    else
    {
        BG3X_internal += rot_B;
        BG3Y_internal += rot_D;
    }
}

void GPU_2D_Engine::draw_rotscale_sprite(uint16_t *attributes, bool objwin)
{
    int flags = (1 << 31);
    const int sprite_sizes[3][8] =
    {
        {1, 1, 2, 2, 4, 4, 8, 8}, //Square
        {2, 1, 4, 1, 4, 2, 8, 4}, //Horizontal
        {1, 2, 1, 4, 2, 4, 4, 8}  //Vertical
    };

    int32_t x = attributes[1] & 0x1FF;
    int32_t y = attributes[0] & 0xFF;

    int shape = (attributes[0] >> 14) & 0x3;
    int size = (attributes[1] >> 14) & 0x3;
    int priority = (attributes[2] >> 10) & 0x3;

    int x_tiles = 0, y_tiles = 0;
    x_tiles = sprite_sizes[shape][(size * 2)];
    y_tiles = sprite_sizes[shape][(size * 2) + 1];

    int width = x_tiles * 8, height = y_tiles * 8;
    int x_bound = x_tiles * 8, y_bound = y_tiles * 8;

    if (attributes[0] & (1 << 9))
    {
        x_bound *= 2;
        y_bound *= 2;
    }

    y = (gpu->get_VCOUNT() - y) & 0xFF;
    if (y >= static_cast<uint32_t>(y_bound))
        return;

    x = static_cast<int32_t>(x << 23) >> 23;

    if (x <= -x_bound)
        return;

    int32_t center_x = x_bound / 2;
    int32_t center_y = y_bound / 2;

    uint32_t x_offset;
    if (x >= 0)
    {
        x_offset = 0;
        if ((x + x_bound) > 256)
           x_bound = 256 - x;
    }
    else
    {
        x_offset = -x;
        x = 0;
    }

    int rot_group = ((attributes[1] >> 9) & 0x1F) * 32;

    int OAM_base = (engine_A) ? 0 : 1024;

    int16_t rot_A = gpu->read_OAM<int16_t>(OAM_base + rot_group + 0x6);
    int16_t rot_B = gpu->read_OAM<int16_t>(OAM_base + rot_group + 0xE);
    int16_t rot_C = gpu->read_OAM<int16_t>(OAM_base + rot_group + 0x16);
    int16_t rot_D = gpu->read_OAM<int16_t>(OAM_base + rot_group + 0x1E);

    int32_t rot_x = ((x_offset - center_x) * rot_A) + ((y - center_y) * rot_B) + (width << 7);
    int32_t rot_y = ((x_offset - center_x) * rot_C) + ((y - center_y) * rot_D) + (height << 7);

    width <<= 8;
    height <<= 8;

    int tile_num = attributes[2] & 0x3FF;

    int y_dimension_num;
    int palette_id = attributes[2] >> 12;
    bool one_palette_mode = attributes[0] & 0x2000;
    int mode = (attributes[0] >> 10) & 0x3;
    if (mode == 1)
        flags |= (1 << 30);
    if (mode == 2)
    {
        if (!objwin)
            return;
        flags |= (1 << 28); //objwin
    }
    if (mode == 3)
    {
        return;
    }
    else
    {
        //Handle one-dimensional/two-dimensional tile mapping
        if (DISPCNT.tile_obj_1d)
        {
            tile_num <<= DISPCNT.tile_obj_1d_bound;
            y_dimension_num = (width >> 11) << one_palette_mode;
        }
        else
            y_dimension_num = 0x20;

        tile_num <<= 5;
        y_dimension_num <<= 5;

        uint32_t pixel_base = tile_num;
        if (engine_A)
            pixel_base += VRAM_OBJA_START;
        else
            pixel_base += VRAM_OBJB_START;

        //printf("\nRot params: %d, %d, %d, %d", rot_A, rot_B, rot_C, rot_D);

        uint16_t color;
        if (one_palette_mode)
        {
            while (x_offset < x_bound)
            {
                if ((uint32_t)rot_x < width && (uint32_t)rot_y < height)
                {
                    uint32_t pixel_address = pixel_base;
                    pixel_address += (rot_y >> 11) * y_dimension_num;
                    pixel_address += (rot_y & 0x700) >> 5;
                    pixel_address += (rot_x >> 11) * 64;
                    pixel_address += (rot_x & 0x700) >> 8;
                    if (engine_A)
                        color = gpu->read_obja<uint8_t>(pixel_address);
                    else
                        color = gpu->read_objb<uint8_t>(pixel_address);

                    if (color && priority <= final_bg_priority[x])
                    {
                        //printf("\nRot params: %d, %d, %d, %d", rot_A, rot_B, rot_C, rot_D);
                        if (DISPCNT.obj_extended_palette)
                        {
                            if (engine_A)
                                color = gpu->read_extpal_obja(palette_id * 512 + color * 2);
                            else
                                color = gpu->read_extpal_objb(palette_id * 512 + color * 2);
                        }
                        else
                        {
                            if (engine_A)
                                color = gpu->read_palette_A(0x200 + (color * 2));
                            else
                                color = gpu->read_palette_B(0x200 + (color * 2));
                        }

                        sprite_scanline[x] = color;
                        sprite_scanline[x] |= flags;
                    }
                }
                rot_x += rot_A;
                rot_y += rot_C;
                x_offset++;
                x++;
            }
        }
        else
        {
            while (x_offset < x_bound)
            {
                if ((uint32_t)rot_x < width && (uint32_t)rot_y < height)
                {
                    uint32_t pixel_address = pixel_base;
                    pixel_address += (rot_y >> 11) * y_dimension_num;
                    pixel_address += (rot_y & 0x700) >> 6;
                    pixel_address += (rot_x >> 11) * 32;
                    pixel_address += (rot_x & 0x700) >> 9;
                    if (engine_A)
                        color = gpu->read_obja<uint8_t>(pixel_address);
                    else
                        color = gpu->read_objb<uint8_t>(pixel_address);

                    if (rot_x & 0x100)
                        color >>= 4;
                    else
                        color &= 0xF;
                    if (color && priority <= final_bg_priority[x])
                    {
                        //printf("\nRot params: %d, %d, %d, %d", rot_A, rot_B, rot_C, rot_D);
                        if (engine_A)
                            sprite_scanline[x] = gpu->read_palette_A(0x200 + (palette_id * 32) + color * 2);
                        else
                            sprite_scanline[x] = gpu->read_palette_B(0x200 + (palette_id * 32) + color * 2);
                        sprite_scanline[x] |= flags;
                    }
                }
                rot_x += rot_A;
                rot_y += rot_C;
                x_offset++;
                x++;
            }
        }
    }
}

void GPU_2D_Engine::draw_sprites(bool objwin)
{
    uint16_t attributes[4];
    uint16_t colors[PIXELS_PER_LINE * 2];
    for (int i = 0; i < PIXELS_PER_LINE * 2; i++)
    {
        colors[i] = 0;
        sprite_scanline[i] = 0;
    }
    int VRAM_obj_base;
    int OAM_base = 0;
    if (engine_A)
    {
        OAM_base = 0;
        VRAM_obj_base = VRAM_OBJA_START;
    }
    else
    {
        OAM_base = 1024;
        VRAM_obj_base = VRAM_OBJB_START;
    }

    const static int sprite_sizes[3][8] =
    {
        {1, 1, 2, 2, 4, 4, 8, 8}, //Square
        {2, 1, 4, 1, 4, 2, 8, 4}, //Horizontal
        {1, 2, 1, 4, 2, 4, 4, 8}  //Vertical
    };
    for (int bg = 3; bg >= 0; bg--)
    {
        for (int obj = 127; obj >= 0; obj--)
        {
            uint32_t flags = (1 << 31); //indicates the pixel has been drawn
            attributes[0] = gpu->read_OAM<uint16_t>(OAM_base + (obj * 8));
            attributes[1] = gpu->read_OAM<uint16_t>(OAM_base + (obj * 8) + 2);
            attributes[2] = gpu->read_OAM<uint16_t>(OAM_base + (obj * 8) + 4);
            attributes[3] = gpu->read_OAM<uint16_t>(OAM_base + (obj * 8) + 6);

            int priority = (attributes[2] >> 10) & 0x3;
            if (priority != bg)
                continue;

            bool rotscale = attributes[0] & (1 << 8);

            if (rotscale)
            {
                draw_rotscale_sprite(attributes, objwin);
                continue;
            }

            if (attributes[0] & (1 << 9))
                continue;

            int x = attributes[1] & 0x1FF;
            int y = attributes[0] & 0xFF;

            y = (gpu->get_VCOUNT() - y) & 0xFF;

            int shape = (attributes[0] >> 14) & 0x3;
            int size = (attributes[1] >> 14) & 0x3;
            int mode = (attributes[0] >> 10) & 0x3;

            if (y >= sprite_sizes[shape][(size * 2) + 1] * 8)
                continue;

            if (mode == 1)
                flags |= (1 << 30); //semitransparent

            if (mode == 2)
            {
                if (!objwin)
                    continue;
                flags |= (1 << 28); //objwin
            }

            if (mode == 3)
            {
                int alpha = attributes[2] >> 12;
                if (!alpha)
                    continue;
                alpha++;
                flags |= (1 << 29) | (alpha << 16);

                int width = sprite_sizes[shape][size * 2] * 8;

                int tile_num = attributes[2] & 0x3FF;
                uint32_t tile_addr;
                if (DISPCNT.bitmap_obj_1d)
                {
                    tile_addr = tile_num * (128 << DISPCNT.bitmap_obj_1d_bound);
                    tile_addr += y * width * 2;
                }
                else
                {
                    if (DISPCNT.bitmap_obj_square)
                    {
                        tile_addr = (tile_num & 0x1F) * 0x10 + (tile_num & 0x3E0) * 0x80;
                        tile_addr += y * 256 * 2;
                    }
                    else
                    {
                        tile_addr = (tile_num & 0xF) * 0x10 + (tile_num & 0x3F0) * 0x80;
                        tile_addr += y * 128 * 2;
                    }
                }

                uint32_t pixel_addr = VRAM_obj_base + tile_addr;
                for (int x_offset = x; x_offset < x + width; x_offset++)
                {
                    if (x_offset >= PIXELS_PER_LINE || bg > final_bg_priority[x_offset])
                        continue;
                    uint16_t color;
                    if (engine_A)
                        color = gpu->read_obja<uint16_t>(pixel_addr);
                    else
                        color = gpu->read_objb<uint16_t>(pixel_addr);
                    pixel_addr += 2;
                    if (color & (1 << 15))
                    {
                        sprite_scanline[x_offset] = color | flags;
                    }
                }
            }
            else
            {
                int x_tiles = 0, y_tiles = 0;
                x_tiles = sprite_sizes[shape][(size * 2)];
                y_tiles = sprite_sizes[shape][(size * 2) + 1];

                if (attributes[1] & 0x2000)
                    y = ((y_tiles * 8) - 1) - y;

                int tile_num = attributes[2] & 0x3FF;
                int palette = attributes[2] >> 12;
                bool one_palette_mode = attributes[0] & (1 << 13);

                int tile_y = y / 8;
                int tile_scanline = y & 0x7;

                if (one_palette_mode)
                    tile_scanline *= 8;
                else
                    tile_scanline *= 4;

                bool x_flip = attributes[1] & (1 << 12);
                int y_dimension_num;
                if (DISPCNT.tile_obj_1d)
                {
                    tile_num <<= DISPCNT.tile_obj_1d_bound;
                    y_dimension_num = x_tiles << one_palette_mode;
                }
                else
                    y_dimension_num = 0x20;

                for (int tile = 0; tile < x_tiles; tile++)
                {
                    int tile_id = tile_y * y_dimension_num;
                    tile_id += ((x_flip) ? (x_tiles - tile - 1) : tile) << one_palette_mode;
                    tile_id += tile_num;

                    tile_id <<= 5;
                    int tile_data = VRAM_obj_base + tile_id + tile_scanline;
                    if (!one_palette_mode)
                    {
                        uint32_t data;
                        if (engine_A)
                            data = gpu->read_obja<uint32_t>(tile_data);
                        else
                            data = gpu->read_objb<uint32_t>(tile_data);
                        int index = (x + (tile * 8)) & 0x1FF;
                        if (x_flip)
                        {
                            for (int i = 0; i < 8; i++)
                                colors[(index + i) & 0x1FF] = (data >> ((7 - i) * 4)) & 0xF;
                        }
                        else
                        {
                            for (int i = 0; i < 8; i++)
                                colors[(index + i) & 0x1FF] = (data >> (i * 4)) & 0xF;
                        }
                        for (int i = 0; i < 8; i++, index = (index + 1) & 0x1FF)
                        {
                            if (index >= PIXELS_PER_LINE)
                                continue;
                            if (!colors[index])
                                continue;
                            if (priority > final_bg_priority[index])
                                continue;
                            if (engine_A)
                                sprite_scanline[index] = gpu->read_palette_A(0x200 + (palette * 32) + colors[index] * 2);
                            else
                                sprite_scanline[index] = gpu->read_palette_B(0x200 + (palette * 32) + colors[index] * 2);
                            sprite_scanline[index] |= flags;
                        }
                    }
                    else
                    {
                        uint64_t data;
                        if (engine_A)
                            data = gpu->read_obja<uint64_t>(tile_data);
                        else
                            data = gpu->read_objb<uint64_t>(tile_data);
                        int index = (x + (tile * 8)) & 0x1FF;
                        if (x_flip)
                        {
                            for (int i = 0; i < 8; i++)
                                colors[(index + i) & 0x1FF] = (data >> ((7 - i) * 8)) & 0xFF;
                        }
                        else
                        {
                            for (int i = 0; i < 8; i++)
                                colors[(index + i) & 0x1FF] = (data >> (i * 8)) & 0xFF;
                        }

                        for (int i = 0; i < 8; i++, index = (index + 1) & 0x1FF)
                        {
                            if (index >= PIXELS_PER_LINE)
                                continue;
                            if (!colors[index])
                                continue;
                            if (priority > final_bg_priority[index])
                                continue;
                            if (engine_A)
                            {
                                if (DISPCNT.obj_extended_palette)
                                    sprite_scanline[index] = gpu->read_extpal_obja((palette * 512) + (colors[index] * 2));
                                else
                                    sprite_scanline[index] = gpu->read_palette_A(0x200 + colors[index] * 2);
                            }

                            else
                            {
                                if (DISPCNT.obj_extended_palette)
                                    sprite_scanline[index] = gpu->read_extpal_objb((palette * 512) + (colors[index] * 2));
                                else
                                    sprite_scanline[index] = gpu->read_palette_B(0x200 + colors[index] * 2);
                            }
                            sprite_scanline[index] |= flags;
                        }
                    }
                }
            }
        }
    }

    if (!objwin)
    {
        for (int i = 0; i < PIXELS_PER_LINE; i++)
        {
            if (!(sprite_scanline[i] & (1 << 31)))
                continue;
            if (!(window_mask[i] & 0x10))
                continue;
            draw_pixel(i, gpu->get_VCOUNT(), sprite_scanline[i] & 0x7FFF, 4);
            if (sprite_scanline[i] & (1 << 30))
                layer_sources[i] |= 0x80; //semitransparent effect
            if (sprite_scanline[i] & (1 << 29))
            {
                //bitmap alpha
                layer_sources[i] = (sprite_scanline[i] >> 16) & 0x1F;
                layer_sources[i] |= 0x80 | 0x40 | 0x10;
            }
        }
    }
}

void GPU_2D_Engine::draw_3D()
{
    eng_3D->render_scanline();
    uint32_t* bark = eng_3D->get_framebuffer();
    int x_offset = BGHOFS[0] & 0x1FF;
    for (int i = 0; i < PIXELS_PER_LINE; i++, x_offset = (x_offset + 1) & 0x1FF)
    {
        if (x_offset >= PIXELS_PER_LINE)
            continue;
        if (!(window_mask[i] & 0x1))
            continue;
        if ((bark[x_offset] & (1 << 31)))
        {
            layer_sources[i + 256] = layer_sources[i];
            layer_sources[i] = 0x41; //account for both BG0 and 3D
            framebuffer[i + (gpu->get_VCOUNT() * PIXELS_PER_LINE)] = bark[x_offset];
            final_bg_priority[i] = BGCNT[0] & 0x3;
        }
    }
}

void GPU_2D_Engine::draw_scanline()
{
    int line = gpu->get_VCOUNT() * PIXELS_PER_LINE;
    for (unsigned int i = 0; i < PIXELS_PER_LINE; i++)
    {
        framebuffer[i + line] = 0xFF000000;
        front_framebuffer[i + line] = 0xFF000000;
        second_layer[i] = 0;
    }

    for (int i = 0; i < PIXELS_PER_LINE * 2; i++)
    {
        layer_sources[i] = 0;
        final_bg_priority[i] = 0xFF;
    }
    draw_backdrop();
    if (DISPCNT.display_win0 || DISPCNT.display_win1 || DISPCNT.obj_win_display)
        get_window_mask();
    else
        memset(window_mask, 0xFF, PIXELS_PER_LINE);
    //TODO: handle mode 6 edge case
    for (int priority = 3; priority >= 0; priority--)
    {
        if (Config::bg_enable[3] && (BGCNT[3] & 0x3) == priority && DISPCNT.display_bg3)
        {
            switch (DISPCNT.bg_mode)
            {
                case 0:
                    draw_bg_txt(3);
                    break;
                case 1:
                case 2:
                    draw_bg_aff(3);
                    break;
                case 3:
                case 4:
                case 5:
                    draw_bg_ext(3);
                    break;
            }
        }
        if (Config::bg_enable[2] && (BGCNT[2] & 0x3) == priority && DISPCNT.display_bg2)
        {
            switch (DISPCNT.bg_mode)
            {
                case 0:
                case 1:
                case 3:
                    draw_bg_txt(2);
                    break;
                case 2:
                case 4:
                    draw_bg_aff(2);
                    break;
                case 5:
                    draw_bg_ext(2);
                    break;
            }
        }
        if (Config::bg_enable[1] && (BGCNT[1] & 0x3) == priority && DISPCNT.display_bg1)
        {
            draw_bg_txt(1);
        }
        if (Config::bg_enable[0] && (BGCNT[0] & 0x3) == priority && DISPCNT.display_bg0)
        {
            if (eng_3D && DISPCNT.bg_3d)
            {
                draw_3D();
            }
            else
                draw_bg_txt(0);
        }
    }
    if (DISPCNT.display_obj)
        draw_sprites(false);
    handle_BLDCNT_effects();

    switch (DISPCNT.display_mode)
    {
        case 0:
            for (int i = 0; i < PIXELS_PER_LINE; i++)
                front_framebuffer[i + line] = 0xFFF3F3F3;
            break;
        case 1:
            for (int i = 0; i < PIXELS_PER_LINE; i++)
            {
                uint8_t r = (framebuffer[i + line] >> 16) & 0x3F;
                uint8_t g = (framebuffer[i + line] >> 8) & 0x3F;
                uint8_t b = (framebuffer[i + line] & 0x3F);
                front_framebuffer[i + line] = 0xFF000000 + ((r << 2) << 16) + ((g << 2) << 8) + (b << 2);
            }
            break;
        case 2:
        {
            uint16_t* VRAM = gpu->get_VRAM_block(DISPCNT.VRAM_block);
            for (int x = 0; x < PIXELS_PER_LINE; x++)
            {
                uint16_t ds_color = VRAM[x + line];
                uint32_t color = 0xFF000000;
                int r = (ds_color & 0x1F) << 3;
                int g = ((ds_color >> 5) & 0x1F) << 3;
                int b = ((ds_color >> 10) & 0x1F) << 3;
                color |= r << 16;
                color |= g << 8;
                color |= b;
                front_framebuffer[x + line] = color;
            }
        }
            break;
        default:
            break;
    }

    //Capture
    if (engine_A && DISPCAPCNT.enable_busy)
    {
        int x_size, y_size;
        switch (DISPCAPCNT.capture_size)
        {
            case 0:
                x_size = 128;
                y_size = 128;
                break;
            case 1:
                x_size = 256;
                y_size = 64;
                break;
            case 2:
                x_size = 256;
                y_size = 128;
                break;
            case 3:
                x_size = 256;
                y_size = 192;
                break;
        }
        if (gpu->get_VCOUNT() < y_size)
        {
            uint32_t read_offset, write_offset;
            if (DISPCNT.display_mode == 2)
                read_offset = 0;
            else
                read_offset = DISPCAPCNT.VRAM_read_offset * 0x4000;
            read_offset += line;
            write_offset = DISPCAPCNT.VRAM_write_offset * 0x4000;
            write_offset += line;
            uint8_t MST;
            switch (DISPCAPCNT.VRAM_write_block)
            {
                case 0:
                    MST = gpu->get_VRAMCNT_A();
                    break;
                case 1:
                    MST = gpu->get_VRAMCNT_B();
                    break;
                case 2:
                    MST = gpu->get_VRAMCNT_C();
                    break;
                case 3:
                    MST = gpu->get_VRAMCNT_D();
                    break;
            }
            //VRAM_dest must be allocated as LCDC-VRAM
            if (!(MST & 0x7))
            {
                uint16_t* VRAM_dest = gpu->get_VRAM_block(DISPCAPCNT.VRAM_write_block);
                for (int x = 0; x < x_size; x++)
                {
                    //TODO: Add main mem FIFO for source B
                    uint32_t source_A;
                    if (DISPCAPCNT.A_3D_only)
                        source_A = eng_3D->get_framebuffer()[x];
                    else
                        source_A = framebuffer[x + line];
                    uint32_t source_B;
                    uint16_t* VRAM = gpu->get_VRAM_block(DISPCNT.VRAM_block);
                    source_B = VRAM[(read_offset + x) & 0xFFFF];

                    int ra = (source_A >> 17) & 0x1F, ga = (source_A >> 9) & 0x1F, ba = (source_A >> 1) & 0x1F;
                    int rb = source_B & 0x1F, gb = (source_B >> 5) & 0x1F, bb = (source_B >> 10) & 0x1F;
                    int rd, gd, bd;

                    uint16_t color = 0;
                    switch (DISPCAPCNT.capture_source)
                    {
                        case 0:
                            rd = ra;
                            gd = ga;
                            bd = ba;
                            break;
                        case 1:
                            rd = rb;
                            gd = gb;
                            bd = bb;
                            break;
                        case 2:
                        case 3:
                            rd = ((ra * DISPCAPCNT.EVA) + (rb * DISPCAPCNT.EVB)) / 16;
                            gd = ((ga * DISPCAPCNT.EVA) + (gb * DISPCAPCNT.EVB)) / 16;
                            bd = ((ba * DISPCAPCNT.EVA) + (bb * DISPCAPCNT.EVB)) / 16;
                            break;
                        default:
                            printf("\nUnrecognized capture source %d", DISPCAPCNT.capture_source);
                            break;
                    }

                    if (rd > 0x1F)
                        rd = 0x1F;
                    if (gd > 0x1F)
                        gd = 0x1F;
                    if (bd > 0x1F)
                        bd = 0x1F;
                    color = rd | (gd << 5) | (bd << 10);
                    VRAM_dest[(write_offset + x) & 0xFFFF] = color | (1 << 15);
                }
            }
        }
    }

    //Apply MASTER_BRIGHT
    int bright_mode = MASTER_BRIGHT >> 14;
    float bright_factor = MASTER_BRIGHT & 0x1F;
    if (bright_factor > 16)
        bright_factor = 16;
    switch (bright_mode)
    {
        case 1:
            for (int i = 0; i < PIXELS_PER_LINE; i++)
            {
                int r = (front_framebuffer[i + line] >> 16) & 0xFF;
                int g = (front_framebuffer[i + line] >> 8) & 0xFF;
                int b = front_framebuffer[i + line] & 0xFF;

                front_framebuffer[i + line] &= ~0xFFFFFF;
                r += ((63 << 2) - r) * (bright_factor / 16);
                g += ((63 << 2) - g) * (bright_factor / 16);
                b += ((63 << 2) - b) * (bright_factor / 16);

                front_framebuffer[i + line] |= (r << 16) | (g << 8) | b;
            }
            break;
        case 2:
            for (int i = 0; i < PIXELS_PER_LINE; i++)
            {
                int r = (front_framebuffer[i + line] >> 16) & 0xFF;
                int g = (front_framebuffer[i + line] >> 8) & 0xFF;
                int b = front_framebuffer[i + line] & 0xFF;

                front_framebuffer[i + line] &= ~0xFFFFFF;
                r -= r * (bright_factor / 16);
                g -= g * (bright_factor / 16);
                b -= b * (bright_factor / 16);

                front_framebuffer[i + line] |= (r << 16) | (g << 8) | b;
            }
            break;
    }

    /*if (engine_A && gpu->get_VCOUNT() == 0)
    {
        for (int i = 0; i < 5; i++)
        {
            printf("\nColor data on scanline 0: $%08X", front_framebuffer[i]);
        }
    }*/
}

void GPU_2D_Engine::handle_BLDCNT_effects()
{
    int scanline = gpu->get_VCOUNT() * PIXELS_PER_LINE;
    for (int pixel = 0; pixel < PIXELS_PER_LINE; pixel++)
    {
        uint8_t r = (framebuffer[pixel + scanline] >> 16) & 0x3F;
        uint8_t g = (framebuffer[pixel + scanline] >> 8) & 0x3F;
        uint8_t b = framebuffer[pixel + scanline] & 0x3F;
        uint16_t blend_factor = BLDY;
        if (blend_factor > 16)
            blend_factor = 16;
        int effect;
        uint16_t BLD_flags = get_BLDCNT();

        uint8_t layer1 = layer_sources[pixel];
        uint8_t layer2 = layer_sources[pixel + 256];
        if (layer2 & 0x40)
            layer2 = 0x1;

        int eva = BLDALPHA & 0x1F;
        int evb = (BLDALPHA >> 8) & 0x1F;

        if (!(window_mask[pixel] & 0x20))
            effect = 0;
        else if ((layer1 & 0x80) && (layer2 & (BLD_flags >> 8)))
        {
            //Sprite blending - semitransparent sprites and bitmap sprites
            effect = 1;

            //additional check for bitmap sprites
            if (layer1 & 0x40)
            {
                eva = layer1 & 0x1F;
                evb = 16 - eva;
            }
        }
        else if ((layer1 & 0x40) && (layer2 & (BLD_flags >> 8)))
        {
            //3D/2D blending
            uint8_t r2 = (second_layer[pixel] >> 16) & 0x3F;
            uint8_t g2 = (second_layer[pixel] >> 8) & 0x3F;
            uint8_t b2 = second_layer[pixel] & 0x3F;
            eva = (framebuffer[pixel + scanline] >> 24) & 0x1F;
            eva++;
            evb = 32 - eva;

            r = ((r * eva) + (r2 * evb)) >> 5;
            g = ((g * eva) + (g2 * evb)) >> 5;
            b = ((b * eva) + (b2 * evb)) >> 5;

            if (eva <= 16)
            {
                r++;
                g++;
                b++;
            }

            if (r > 0x3F)
                r = 0x3F;
            if (g > 0x3F)
                g = 0x3F;
            if (b > 0x3F)
                b = 0x3F;

            framebuffer[pixel + scanline] = 0xFF000000 | (r << 16) | (g << 8) | b;
            continue;
        }
        else if (layer1 & (BLD_flags & 0xFF))
        {
            if (BLDCNT.effect == 1 && (layer2 & (BLD_flags >> 8)))
                effect = 1;
            else if (BLDCNT.effect >= 2)
                effect = BLDCNT.effect;
            else
                effect = 0;
        }
        else
            effect = 0;

        switch (effect)
        {
            case 1:
            {
                uint8_t r2 = (second_layer[pixel] >> 16) & 0x3F;
                uint8_t g2 = (second_layer[pixel] >> 8) & 0x3F;
                uint8_t b2 = second_layer[pixel] & 0x3F;
                if (eva > 16)
                    eva = 16;
                if (evb > 16)
                    evb = 16;

                r = std::min(63, ((r * eva) + (r2 * evb)) >> 4);
                g = std::min(63, ((g * eva) + (g2 * evb)) >> 4);
                b = std::min(63, ((b * eva) + (b2 * evb)) >> 4);
            }
                break;
            case 2:
                r += ((63 - r) * blend_factor) >> 4;
                g += ((63 - g) * blend_factor) >> 4;
                b += ((63 - b) * blend_factor) >> 4;
                break;
            case 3:
                r -= (r * blend_factor) >> 4;
                g -= (g * blend_factor) >> 4;
                b -= (b * blend_factor) >> 4;
                break;
        }
        framebuffer[pixel + scanline] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

void GPU_2D_Engine::get_framebuffer(uint32_t* buffer)
{
    for (int y = 0; y < SCANLINES; y++)
    {
        for (int x = 0; x < PIXELS_PER_LINE; x++)
            buffer[x + (y * PIXELS_PER_LINE)] = front_framebuffer[x + (y * PIXELS_PER_LINE)];
    }

    /*if (engine_A)
    {
        for (int i = 0; i < 5; i++)
            printf("\nEngine A final color data: $%08X", buffer[i]);
    }*/
}

void GPU_2D_Engine::set_framebuffer(uint32_t *buffer)
{
    //framebuffer = buffer;
}

void GPU_2D_Engine::clear_buffer()
{
    for (int y = 0; y < SCANLINES; y++)
    {
        for (int x = 0; x < PIXELS_PER_LINE; x++)
        {
            framebuffer[x + (y * PIXELS_PER_LINE)] = 0xFF000000;
            front_framebuffer[x + (y * PIXELS_PER_LINE)] = 0xFF000000;
        }
    }
}

uint32_t GPU_2D_Engine::get_DISPCNT()
{
    uint32_t reg = 0;
    reg |= DISPCNT.bg_mode;
    reg |= DISPCNT.bg_3d << 3;
    reg |= DISPCNT.tile_obj_1d << 4;
    reg |= DISPCNT.bitmap_obj_square << 5;
    reg |= DISPCNT.bitmap_obj_1d << 6;
    reg |= DISPCNT.display_bg0 << 8;
    reg |= DISPCNT.display_bg1 << 9;
    reg |= DISPCNT.display_bg2 << 10;
    reg |= DISPCNT.display_bg3 << 11;
    reg |= DISPCNT.display_obj << 12;
    reg |= DISPCNT.display_win0 << 13;
    reg |= DISPCNT.display_win1 << 14;
    reg |= DISPCNT.obj_win_display << 15;
    reg |= DISPCNT.display_mode << 16;
    reg |= DISPCNT.VRAM_block << 18;
    reg |= DISPCNT.tile_obj_1d_bound << 20;
    reg |= DISPCNT.bitmap_obj_1d_bound << 22;
    reg |= DISPCNT.hblank_obj_processing << 23;
    reg |= DISPCNT.char_base << 24;
    reg |= DISPCNT.screen_base << 27;
    reg |= DISPCNT.bg_extended_palette << 30;
    reg |= DISPCNT.obj_extended_palette << 31;
    return reg;
}

uint16_t GPU_2D_Engine::get_BGCNT(int index)
{
    return BGCNT[index];
}

uint16_t GPU_2D_Engine::get_BGHOFS(int index)
{
    return BGHOFS[index];
}

uint16_t GPU_2D_Engine::get_BGVOFS(int index)
{
    return BGVOFS[index];
}

uint32_t GPU_2D_Engine::get_BG2X()
{
    return BG2X;
}

uint16_t GPU_2D_Engine::get_WIN0V()
{
    return WIN0V;
}

uint16_t GPU_2D_Engine::get_WIN1V()
{
    return WIN1V;
}

uint16_t GPU_2D_Engine::get_WININ()
{
    uint16_t reg = 0;
    for (int bit = 0; bit < 4; bit++)
    {
        reg |= WININ.win0_bg_enabled[bit] << bit;
        reg |= WININ.win1_bg_enabled[bit] << (bit + 8);
    }
    reg |= WININ.win0_obj_enabled << 4;
    reg |= WININ.win0_color_special << 5;
    reg |= WININ.win1_obj_enabled << 12;
    reg |= WININ.win1_color_special << 13;
    return reg;
}

uint16_t GPU_2D_Engine::get_WINOUT()
{
    uint16_t reg = 0;
    for (int bit = 0; bit < 4; bit++)
    {
        reg |= WINOUT.outside_bg_enabled[bit] << bit;
        reg |= WINOUT.objwin_bg_enabled[bit] << (bit + 8);
    }
    reg |= WINOUT.outside_obj_enabled << 4;
    reg |= WINOUT.outside_color_special << 5;
    reg |= WINOUT.objwin_obj_enabled << 12;
    reg |= WINOUT.objwin_color_special << 13;
    return reg;
}

uint16_t GPU_2D_Engine::get_BLDCNT()
{
    uint16_t reg = 0;
    for (int bit = 0; bit < 4; bit++)
    {
        reg |= BLDCNT.bg_first_target_pix[bit] << bit;
        reg |= BLDCNT.bg_second_target_pix[bit] << (bit + 8);
    }
    reg |= BLDCNT.obj_first_target_pix << 4;
    reg |= BLDCNT.obj_second_target_pix << 12;

    reg |= BLDCNT.bd_first_target_pix << 5;
    reg |= BLDCNT.bd_second_target_pix << 13;

    reg |= BLDCNT.effect << 6;
    return reg;
}

uint16_t GPU_2D_Engine::get_BLDALPHA()
{
    return BLDALPHA;
}

uint16_t GPU_2D_Engine::get_MASTER_BRIGHT()
{
    return MASTER_BRIGHT;
}

uint32_t GPU_2D_Engine::get_DISPCAPCNT()
{
    if (!engine_A)
        return 0;

    uint32_t reg = 0;
    reg |= DISPCAPCNT.EVA;
    reg |= DISPCAPCNT.EVB << 8;
    reg |= DISPCAPCNT.VRAM_write_block << 16;
    reg |= DISPCAPCNT.VRAM_write_offset << 18;
    reg |= DISPCAPCNT.capture_size << 20;
    reg |= DISPCAPCNT.A_3D_only << 24;
    reg |= DISPCAPCNT.B_display_FIFO << 25;
    reg |= DISPCAPCNT.VRAM_read_offset << 26;
    reg |= DISPCAPCNT.capture_source << 29;
    reg |= DISPCAPCNT.enable_busy << 31;
    return reg;
}

void GPU_2D_Engine::set_eng_3D(GPU_3D *eng_3D)
{
    this->eng_3D = eng_3D;
}

//TODO: handle forced blank bit 7?
void GPU_2D_Engine::set_DISPCNT_lo(uint16_t halfword)
{
    DISPCNT.bg_mode = halfword & 0x7;
    DISPCNT.bg_3d = halfword & (1 << 3);
    DISPCNT.tile_obj_1d = halfword & (1 << 4);
    DISPCNT.bitmap_obj_square = halfword & (1 << 5);
    DISPCNT.bitmap_obj_1d = halfword & (1 << 6);
    DISPCNT.display_bg0 = halfword & (1 << 8);
    DISPCNT.display_bg1 = halfword & (1 << 9);
    DISPCNT.display_bg2 = halfword & (1 << 10);
    DISPCNT.display_bg3 = halfword & (1 << 11);
    DISPCNT.display_obj = halfword & (1 << 12);
    DISPCNT.display_win0 = halfword & (1 << 13);
    DISPCNT.display_win1 = halfword & (1 << 14);
    DISPCNT.obj_win_display = halfword & (1 << 15);
}

void GPU_2D_Engine::set_DISPCNT(uint32_t word)
{
    set_DISPCNT_lo(word & 0xFFFF);
    DISPCNT.display_mode = (word >> 16) & 0x3;
    DISPCNT.VRAM_block = (word >> 18) & 0x3;
    DISPCNT.tile_obj_1d_bound = (word >> 20) & 0x3;
    DISPCNT.bitmap_obj_1d_bound = word & (1 << 22);
    DISPCNT.hblank_obj_processing = word & (1 << 23);
    DISPCNT.char_base = (word >> 24) & 0x7;
    DISPCNT.screen_base = (word >> 27) & 0x7;
    DISPCNT.bg_extended_palette = word & (1 << 30);
    DISPCNT.obj_extended_palette = word & (1 << 31);
}

void GPU_2D_Engine::set_BGCNT(uint16_t halfword, int index)
{
    BGCNT[index] = halfword;
}

void GPU_2D_Engine::set_BGHOFS(uint16_t halfword, int index)
{
    BGHOFS[index] = halfword;
}

void GPU_2D_Engine::set_BGVOFS(uint16_t halfword, int index)
{
    BGVOFS[index] = halfword;
}

void GPU_2D_Engine::set_BG2P(uint16_t halfword, int index)
{
    BG2P[index] = halfword;
    if (gpu->get_VCOUNT() < 192)
        BG2P_internal[index] = halfword;
}

void GPU_2D_Engine::set_BG3P(uint16_t halfword, int index)
{
    BG3P[index] = halfword;
    if (gpu->get_VCOUNT() < 192)
        BG3P_internal[index] = halfword;
}

void GPU_2D_Engine::set_BG2X(uint32_t word)
{
    BG2X = word;
    if (gpu->get_VCOUNT() < 192)
        BG2X_internal = word;
}

void GPU_2D_Engine::set_BG2Y(uint32_t word)
{
    BG2Y = word;
    if (gpu->get_VCOUNT() < 192)
        BG2Y_internal = word;
}

void GPU_2D_Engine::set_BG3X(uint32_t word)
{
    BG3X = word;
    if (gpu->get_VCOUNT() < 192)
        BG3X_internal = word;
}

void GPU_2D_Engine::set_BG3Y(uint32_t word)
{
    BG3Y = word;
    if (gpu->get_VCOUNT() < 192)
        BG3Y_internal = word;
}

void GPU_2D_Engine::set_WIN0H(uint16_t halfword)
{
    WIN0H = halfword;
}

void GPU_2D_Engine::set_WIN1H(uint16_t halfword)
{
    WIN1H = halfword;
}

void GPU_2D_Engine::set_WIN0V(uint16_t halfword)
{
    WIN0V = halfword;
}

void GPU_2D_Engine::set_WIN1V(uint16_t halfword)
{
    WIN1V = halfword;
}

void GPU_2D_Engine::set_WININ(uint16_t halfword)
{
    for (int bit = 0; bit < 4; bit++)
    {
        WININ.win0_bg_enabled[bit] = halfword & (1 << bit);
        WININ.win1_bg_enabled[bit] = halfword & (1 << (bit + 8));
    }
    WININ.win0_obj_enabled = halfword & (1 << 4);
    WININ.win0_color_special = halfword & (1 << 5);

    WININ.win1_obj_enabled = halfword & (1 << 12);
    WININ.win1_color_special = halfword & (1 << 13);
}

void GPU_2D_Engine::set_WINOUT(uint16_t halfword)
{
    for (int bit = 0; bit < 4; bit++)
    {
        WINOUT.outside_bg_enabled[bit] = halfword & (1 << bit);
        WINOUT.objwin_bg_enabled[bit] = halfword & (1 << (bit + 8));
    }
    WINOUT.outside_obj_enabled = halfword & (1 << 4);
    WINOUT.outside_color_special = halfword & (1 << 5);

    WINOUT.objwin_obj_enabled = halfword & (1 << 12);
    WINOUT.objwin_color_special = halfword & (1 << 13);
}

void GPU_2D_Engine::set_MOSAIC(uint16_t halfword)
{
    MOSAIC = halfword;
}

void GPU_2D_Engine::set_BLDCNT(uint16_t halfword)
{
    printf("\nSet BLDCNT: $%04X", halfword);
    for (int bit = 0; bit < 4; bit++)
    {
        BLDCNT.bg_first_target_pix[bit] = halfword & (1 << bit);
        BLDCNT.bg_second_target_pix[bit] = halfword & (1 << (bit + 8));
    }
    BLDCNT.obj_first_target_pix = halfword & (1 << 4);
    BLDCNT.obj_second_target_pix = halfword & (1 << 12);

    BLDCNT.bd_first_target_pix = halfword & (1 << 5);
    BLDCNT.bd_second_target_pix = halfword & (1 << 13);

    BLDCNT.effect = (halfword >> 6) & 0x3;
}

void GPU_2D_Engine::set_BLDALPHA(uint16_t halfword)
{
    printf("\nSet BLDALPHA: $%04X", halfword);
    BLDALPHA = halfword;
}

void GPU_2D_Engine::set_BLDY(uint8_t byte)
{
    BLDY = byte;
}

void GPU_2D_Engine::set_MASTER_BRIGHT(uint16_t halfword)
{
    MASTER_BRIGHT = halfword;
}

void GPU_2D_Engine::set_DISPCAPCNT(uint32_t word)
{
    if (!engine_A)
        return;

    DISPCAPCNT.EVA = word & 0x1F;
    if (DISPCAPCNT.EVA > 16)
        DISPCAPCNT.EVA = 16;
    DISPCAPCNT.EVB = (word >> 8) & 0x1F;
    if (DISPCAPCNT.EVB > 16)
        DISPCAPCNT.EVB = 16;
    DISPCAPCNT.VRAM_write_block = (word >> 16) & 0x3;
    DISPCAPCNT.VRAM_write_offset = (word >> 18) & 0x3;
    DISPCAPCNT.capture_size = (word >> 20) & 0x3;
    DISPCAPCNT.A_3D_only = word & (1 << 24);
    DISPCAPCNT.B_display_FIFO = word & (1 << 25);
    DISPCAPCNT.VRAM_read_offset = (word >> 26) & 0x3;
    DISPCAPCNT.capture_source = (word >> 29) & 0x3;
    if (!DISPCAPCNT.enable_busy && (word & (1 << 31)))
        captured_lines = -1;
    DISPCAPCNT.enable_busy = word & (1 << 31);
}
