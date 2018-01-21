#include "../emulator.hpp"
#include "../gpu.hpp"

void GPU::gba_run(uint64_t c)
{
    uint64_t new_cycles = cycles + c;
    if (cycles < 960 && new_cycles >= 960)
    {
        //HBLANK
        //printf("\nHBLANK!");
        DISPSTAT7.is_HBLANK = true;
        if (VCOUNT < GBA_SCANLINES)
        {
            eng_A.gba_draw_scanline();
            e->HBLANK_DMA_request();
        }
        if (DISPSTAT7.IRQ_on_HBLANK)
            e->request_interrupt_gba(1);
    }
    if (cycles < 1232 && new_cycles >= 1232)
    {
        //End of HBLANK
        DISPSTAT7.is_HBLANK = false;
        VCOUNT++;
        if (VCOUNT == GBA_SCANLINES)
        {
            //VBLANK
            printf("\nVBLANK!");
            eng_A.VBLANK_start();
            DISPSTAT7.is_VBLANK = true;
            frame_complete = true;
            if (DISPSTAT7.IRQ_on_VBLANK)
                e->request_interrupt_gba(0);
        }
        if (VCOUNT == 228)
        {
            DISPSTAT7.is_VBLANK = false;
            VCOUNT = 0;
        }
        new_cycles -= 1232;
    }
    cycles = new_cycles;
}

void GPU_2D_Engine::gba_draw_scanline()
{
    int line_offset = gpu->get_VCOUNT() * PIXELS_PER_LINE;
    for (int i = 0; i < GBA_PIXELS_PER_LINE; i++)
    {
        final_bg_priority[i] = 0xFF;
        framebuffer[i + line_offset] = 0xFF000000;
    }
    //Backdrop
    uint16_t* palette = gpu->get_palette(engine_A);
    for (int x = 0; x < GBA_PIXELS_PER_LINE; x++)
        gba_draw_pixel(x, gpu->get_VCOUNT(), palette[0], 5);
    for (int priority = 3; priority >= 0; priority--)
    {
        if (DISPCNT.display_bg3 && (BGCNT[3] & 0x3) == priority)
        {
            switch (DISPCNT.bg_mode)
            {
                case 0:
                    gba_draw_txt(3);
                    break;
                case 2:
                    gba_draw_mode2(3);
                    break;
            }
        }
        if (DISPCNT.display_bg2 && (BGCNT[2] & 0x3) == priority)
        {
            switch (DISPCNT.bg_mode)
            {
                case 0:
                    gba_draw_txt(2);
                    break;
                case 1:
                case 2:
                    gba_draw_mode2(2);
                    break;
            }
        }
        if (DISPCNT.display_bg1 && (BGCNT[1] & 0x3) == priority)
        {
            switch (DISPCNT.bg_mode)
            {
                case 0:
                case 1:
                    gba_draw_txt(1);
                    break;
            }
        }
        if (DISPCNT.display_bg0 && (BGCNT[0] & 0x3) == priority)
        {
            switch (DISPCNT.bg_mode)
            {
                case 0:
                case 1:
                    gba_draw_txt(0);
                    break;
            }
        }
    }
    if (DISPCNT.display_obj)
        gba_draw_sprites();

    gba_handle_BLDCNT();

    int centered_x = ((PIXELS_PER_LINE - GBA_PIXELS_PER_LINE) / 2);
    int centered_y = ((SCANLINES - GBA_SCANLINES) / 2) * PIXELS_PER_LINE;
    for (int i = 0; i < GBA_PIXELS_PER_LINE; i++)
    {
        uint32_t r = (framebuffer[i + line_offset] & 0x001F0000) << 3;
        uint32_t g = (framebuffer[i + line_offset] & 0x00001F00) << 3;
        uint32_t b = (framebuffer[i + line_offset] & 0x0000001F) << 3;
        front_framebuffer[i + centered_x + centered_y + line_offset] = 0xFF000000 | r | g | b;
    }
}

void GPU_2D_Engine::gba_draw_txt(int index)
{
    uint32_t base = 0x06000000;
    uint32_t screen_base = base + ((BGCNT[index] >> 8) & 0x1F) * 1024 * 2;
    uint32_t char_base = base + ((BGCNT[index] >> 2) & 0x3) * 1024 * 16;
    int line = gpu->get_VCOUNT();

    int x_mask = 0xFF, y_mask = 0xFF;
    if (BGCNT[index] & (1 << 14))
        x_mask = 0x1FF;
    if (BGCNT[index] & (1 << 15))
        y_mask = 0x1FF;
    uint16_t x_offset = BGHOFS[index] & x_mask;
    uint16_t y_offset = (BGVOFS[index] + line) & y_mask;

    bool one_palette_mode = BGCNT[index] & (1 << 7);

    for (int i = 0; i < GBA_PIXELS_PER_LINE; i++)
    {
        uint32_t tile_offset = ((x_offset >> 3) + ((y_offset >> 3) * 32)) * 2;
        uint16_t tile_info = gpu->read_gba<uint16_t>(screen_base + tile_offset);
        uint16_t tile = tile_info & 0x3FF;
        bool x_flip = tile_info & (1 << 10);
        bool y_flip = tile_info & (1 << 11);
        int palette = tile_info >> 12;

        uint32_t data_offset = tile * 0x20;
        uint8_t data;
        if (one_palette_mode)
        {
            data_offset <<= 1;
            palette = 0;
            if (x_flip)
                data_offset += 7 - (x_offset & 0x7);
            else
                data_offset += x_offset & 0x7;

            if (y_flip)
                data_offset += (7 * 8) - ((y_offset & 0x7) * 8);
            else
                data_offset += (y_offset & 0x7) * 8;

            data = gpu->read_gba<uint8_t>(char_base + data_offset);
        }
        else
        {
            if (x_flip)
                data_offset += 3 - ((x_offset / 2) & 0x3);
            else
                data_offset += (x_offset / 2) & 0x3;

            if (y_flip)
                data_offset += (7 * 4) - ((y_offset & 0x7) * 4);
            else
                data_offset += (y_offset & 0x7) * 4;

            data = gpu->read_gba<uint8_t>(char_base + data_offset);
            data >>= 4 * ((x_offset & 0x1) ^ x_flip);
            data &= 0xF;
        }

        if (data)
        {
            uint16_t color = gpu->read_palette_A((palette * 16 + data) * 2);
            gba_draw_pixel(i, line, color, index);
            final_bg_priority[i] = BGCNT[index] & 0x3;
        }

        x_offset = (x_offset + 1) & x_mask;
    }
}

//Tile-based affine backgrounds
void GPU_2D_Engine::gba_draw_mode2(int index)
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

    uint32_t screen_base = 0x06000000;
    uint32_t char_base = 0x06000000;

    screen_base += ((BGCNT[index] >> 8) & 0x1F) * 1024 * 2;
    char_base += ((BGCNT[index] >> 2) & 0x3) * 1024 * 16;

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
    for (int pixel = 0; pixel < GBA_PIXELS_PER_LINE; pixel++)
    {
        //if (window_mask[pixel] & (1 << index))
        //{
            if ((!((x_offset | y_offset) & overflow_mask)))
            {
                uint16_t tile;
                uint16_t color;
                uint32_t tile_addr = ((((y_offset & mask) >> 11) << y_factor) + ((x_offset & mask) >> 11));

                uint32_t tile_x_offset = (x_offset >> 8) & 0x7;
                uint32_t tile_y_offset = (y_offset >> 8) & 0x7;
                tile = gpu->read_gba<uint8_t>(screen_base + tile_addr);

                uint32_t char_addr = (tile << 6) + (tile_y_offset << 3) + tile_x_offset;
                color = gpu->read_gba<uint8_t>(char_base + char_addr);

                if (color)
                {
                    color = palette[color];
                    gba_draw_pixel(pixel, gpu->get_VCOUNT(), color, index);
                    final_bg_priority[pixel] = BGCNT[index] & 0x3;
                }
            }
        //}
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

void GPU_2D_Engine::gba_draw_sprites()
{
    uint16_t attributes[4];
    uint16_t colors[PIXELS_PER_LINE * 2];
    for (int i = 0; i < PIXELS_PER_LINE * 2; i++)
    {
        colors[i] = 0;
        sprite_scanline[i] = 0;
    }

    const static int sprite_sizes[3][8] =
    {
        {1, 1, 2, 2, 4, 4, 8, 8}, //Square
        {2, 1, 4, 1, 4, 2, 8, 4}, //Horizontal
        {1, 2, 1, 4, 2, 4, 4, 8}  //Vertical
    };

    //The GBA is flawed when it comes to sprite priority
    //OBJ0 will ALWAYS have priority over OBJ1-127, regardless of the BG priority
    //This allows for invalid priorities to be specified
    for (int obj = 127; obj >= 0; obj--)
    {
        uint32_t flags = (1 << 31); //indicates the pixel has been drawn
        attributes[0] = gpu->read_OAM<uint16_t>(obj * 8);
        attributes[1] = gpu->read_OAM<uint16_t>(obj * 8 + 2);
        attributes[2] = gpu->read_OAM<uint16_t>(obj * 8 + 4);

        int bg_priority = (attributes[2] >> 10) & 0x3;
        bool rotscale = attributes[0] & (1 << 8);

        if (rotscale)
        {
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
            y_dimension_num = x_tiles << one_palette_mode;
        else
            y_dimension_num = 0x20;

        for (int tile = 0; tile < x_tiles; tile++)
        {
            int tile_id = tile_y * y_dimension_num;
            tile_id += ((x_flip) ? (x_tiles - tile - 1) : tile) << one_palette_mode;
            tile_id += tile_num;

            tile_id <<= 5;
            int tile_data = 0x06010000 + tile_id + tile_scanline;
            if (!one_palette_mode)
            {
                uint32_t data;
                data = gpu->read_gba<uint32_t>(tile_data);
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
                    if (index >= GBA_PIXELS_PER_LINE)
                        continue;
                    if (!colors[index])
                        continue;
                    if (bg_priority > final_bg_priority[index])
                        continue;
                    sprite_scanline[index] = gpu->read_palette_A(0x200 + (palette * 32) + colors[index] * 2);
                    sprite_scanline[index] |= flags;
                }
            }
            else
            {
                uint64_t data;
                data = gpu->read_gba<uint64_t>(tile_data);
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
                    if (index >= GBA_PIXELS_PER_LINE)
                        continue;
                    if (!colors[index])
                        continue;
                    if (bg_priority > final_bg_priority[index])
                        continue;
                    sprite_scanline[index] = gpu->read_palette_A(0x200 + colors[index] * 2);
                    sprite_scanline[index] |= flags;
                }
            }
        }
    }

    for (int i = 0; i < GBA_PIXELS_PER_LINE; i++)
    {
        if (!(sprite_scanline[i] & (1 << 31)))
            continue;
        gba_draw_pixel(i, gpu->get_VCOUNT(), sprite_scanline[i] & 0x7FFF, 4);
    }
}

//TODO: Does the GBA convert 15-bit colors to 18-bit in the same manner the DS does?
//This function assumes they don't.
void GPU_2D_Engine::gba_draw_pixel(int x, int y, uint16_t color, int source)
{
    int pixel = x + (y * PIXELS_PER_LINE);
    uint8_t r = (color & 0x1F);
    uint8_t g = ((color >> 5) & 0x1F);
    uint8_t b = ((color >> 10) & 0x1F);
    uint32_t new_color = 0xFF000000 | (r << 16) | (g << 8) | b;
    layer_sources[x + 256] = layer_sources[x];
    second_layer[x] = framebuffer[pixel];
    layer_sources[x] = (1 << source);
    framebuffer[pixel] = new_color;
}

void GPU_2D_Engine::gba_handle_BLDCNT()
{
    int scanline = gpu->get_VCOUNT() * PIXELS_PER_LINE;
    for (int pixel = 0; pixel < GBA_PIXELS_PER_LINE; pixel++)
    {
        uint8_t r = (framebuffer[pixel + scanline] >> 16) & 0x1F;
        uint8_t g = (framebuffer[pixel + scanline] >> 8) & 0x1F;
        uint8_t b = framebuffer[pixel + scanline] & 0x1F;
        uint16_t blend_factor = BLDY;
        if (blend_factor > 16)
            blend_factor = 16;
        int effect;
        uint16_t BLD_flags = get_BLDCNT();

        uint8_t layer1 = layer_sources[pixel];
        uint8_t layer2 = layer_sources[pixel + 256];

        int eva = BLDALPHA & 0x1F;
        int evb = (BLDALPHA >> 8) & 0x1F;

        //if (!(window_mask[pixel] & 0x20))
            //effect = 0;
        if ((layer1 & 0x80) && (layer2 & (BLD_flags >> 8)))
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
                uint8_t r2 = (second_layer[pixel] >> 16) & 0x1F;
                uint8_t g2 = (second_layer[pixel] >> 8) & 0x1F;
                uint8_t b2 = second_layer[pixel] & 0x1F;
                if (eva > 16)
                    eva = 16;
                if (evb > 16)
                    evb = 16;

                r = std::min(0x1F, ((r * eva) + (r2 * evb)) >> 4);
                g = std::min(0x1F, ((g * eva) + (g2 * evb)) >> 4);
                b = std::min(0x1F, ((b * eva) + (b2 * evb)) >> 4);
            }
                break;
            case 2:
                r += ((0x1F - r) * blend_factor) >> 4;
                g += ((0x1F - g) * blend_factor) >> 4;
                b += ((0x1F - b) * blend_factor) >> 4;
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

void GPU::gba_set_DISPCNT(uint16_t halfword)
{
    eng_A.gba_set_DISPCNT(halfword);
}

void GPU_2D_Engine::gba_set_DISPCNT(uint16_t halfword)
{
    printf("\nDISPCNT: $%04X", halfword);
    DISPCNT.bg_mode = halfword & 0x7;
    DISPCNT.tile_obj_1d = halfword & (1 << 6);
    DISPCNT.display_bg0 = halfword & (1 << 8);
    DISPCNT.display_bg1 = halfword & (1 << 9);
    DISPCNT.display_bg2 = halfword & (1 << 10);
    DISPCNT.display_bg3 = halfword & (1 << 11);
    DISPCNT.display_obj = halfword & (1 << 12);
    DISPCNT.display_win0 = halfword & (1 << 13);
    DISPCNT.display_win1 = halfword & (1 << 14);
    DISPCNT.obj_win_display = halfword & (1 << 15);
}
