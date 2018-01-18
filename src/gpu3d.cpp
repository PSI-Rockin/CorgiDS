/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "emulator.hpp"
#include "gpu3d.hpp"

//Macro that enables printf statements for geometry commands
//#define GX_DEBUG

using namespace std;

const uint8_t GPU_3D::cmd_param_amounts[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 1, 1, 1, 0, 16, 12, 16, 12, 9, 3, 3, 0, 0, 0,
    1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const uint16_t GPU_3D::cmd_cycle_amounts[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 17, 36, 17, 36, 19, 34, 30, 35, 31, 28, 22, 22, 0, 0, 0,
    1, 9, 1, 9, 8, 8, 8, 8, 8, 1, 1, 1, 0, 0, 0, 0,
    4, 4, 6, 1, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    392, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    103, 9, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void MTX::set(const MTX &mtx)
{
    //printf("\nSize of MTX: %d", sizeof(MTX));
    memcpy(m, mtx.m, sizeof(MTX));
}

const MTX GPU_3D::IDENTITY =
{
    {
        {1 << 12, 0, 0, 0},
        {0, 1 << 12, 0, 0},
        {0, 0, 1 << 12, 0},
        {0, 0, 0, 1 << 12}
    }
};

GPU_3D::GPU_3D(Emulator* e, GPU* gpu) : e(e), gpu(gpu)
{
    vert_bank1 = new Vertex[6144];
    vert_bank2 = new Vertex[6144];
    poly_bank1 = new Polygon[2048];
    poly_bank2 = new Polygon[2048];
}

GPU_3D::~GPU_3D()
{
    delete[] vert_bank1;
    delete[] vert_bank2;
    delete[] poly_bank1;
    delete[] poly_bank2;
}

void GPU_3D::power_on()
{
    //Clear GXPIPE and GXFIFO
    queue<GX_Command> empty, empty2;
    swap(empty, GXPIPE);
    swap(empty2, GXFIFO);
    set_DISP3DCNT(0);
    get_identity_mtx(mult_params);
    get_identity_mtx(projection_mtx);
    get_identity_mtx(vector_mtx);
    get_identity_mtx(modelview_mtx);
    get_identity_mtx(texture_mtx);
    mult_params_index = 0;
    geo_vert_count = 0;
    geo_poly_count = 0;
    rend_vert_count = 0;
    rend_poly_count = 0;
    VTX_16_index = 0;
    modelview_sp = 0;
    projection_sp = 0;
    vertex_list_count = 0;
    clip_dirty = true;
    geo_vert = vert_bank1;
    rend_vert = vert_bank2;
    geo_poly = poly_bank1;
    rend_poly = poly_bank2;

    cycles = 0;
    param_count = 0;
    cmd_param_count = 0;
    cmd_count = 0;
    swap_buffers = false;
    GXSTAT.boxtest_result = false;
    GXSTAT.box_pos_vec_busy = false;
    GXSTAT.mtx_stack_busy = false;
    GXSTAT.mtx_overflow = false;
    GXSTAT.geo_busy = false;
    GXSTAT.GXFIFO_irq_stat = 0;
    TEXIMAGE_PARAM.format = 0;

    current_color = 0x7FFF;
    CLEAR_DEPTH = 0x7FFF;

    last_poly_strip = nullptr;
    previous_shadow_mask = false;
    memset(stencil_buffer, 0, sizeof(stencil_buffer));
}

//Applies perspective correction interpolation on a pixel with attributes u1, u2
//TODO: apply the actual GPU algorithm, which takes shortcuts. This is the "normal" method
int64_t GPU_3D::interpolate(uint64_t pixel, uint64_t pixel_range, int64_t u1, int64_t u2, int32_t w1, int32_t w2)
{
    if (!pixel_range)
        return u1;
    int64_t bark = 0;
    bark = (pixel_range - pixel) * (u1 * w2);
    bark += pixel * (u2 * w1);
    int64_t denom = (pixel_range - pixel) * w2;
    denom += pixel * w1;
    /*if (denom == 0)
        return 0; //Dunno lol*/
    return bark / denom;
}

//((1-a)(u0*w1) + a(u1*w0)) / ((1-a)*w1 + a*w0)
//finalZ = (((vertexZ * 0x4000) / vertexW) + 0x3FFF) * 0x200
void GPU_3D::render_scanline()
{
    int line = gpu->get_VCOUNT();
    //Draw the rear-plane
    //X=(X*200h)+((X+1)/8000h)*1FFh
    uint32_t rear_z = (CLEAR_DEPTH * 0x200) + ((CLEAR_DEPTH + 1) / 0x8000) * 0x1FF;
    bool rear_plane_fog = CLEAR_COLOR & (1 << 15);
    int rear_a = (CLEAR_COLOR >> 16) & 0x1F;
    uint32_t rear_color = rear_a << 24;
    for (int i = 0; i < PIXELS_PER_LINE; i++)
    {
        framebuffer[i] = rear_color;
        z_buffer[line][i] = rear_z;
        attr_buffer[i].opaque_id = (CLEAR_COLOR >> 24) & 0x1F;
        attr_buffer[i].fog = rear_plane_fog;
        attr_buffer[i].translucent = false;
    }
    int y_coord = line * PIXELS_PER_LINE;
    for (int i = 0; i < rend_poly_count; i++)
    {
        Polygon* current_poly = &rend_poly[i];
        if (line < current_poly->top_y || line > current_poly->bottom_y)
            continue;

        if (current_poly->shadow_mask)
        {
            render_shadow_mask(current_poly);
            continue;
        }

        previous_shadow_mask = false;

        int left_x = 512, right_x = -512;
        uint32_t left_r = 0, left_g = 0, left_b = 0,
                 right_r = 0, right_g = 0, right_b = 0;
        uint32_t left_z, right_z;
        int32_t left_w, right_w;
        int16_t left_s, left_t, right_s, right_t;

        //Figure out the leftmost/rightmost points on the polygon on this scanline
        //Using a variation of Bresenham's line algorithm
        for (int vert = 0; vert < current_poly->vert_count; vert++)
        {
            int vert2 = (vert + 1) % current_poly->vert_count;
            int x1 = current_poly->vertices[vert]->final_pos[0],
                y1 = current_poly->vertices[vert]->final_pos[1],
                x2 = current_poly->vertices[vert2]->final_pos[0],
                y2 = current_poly->vertices[vert2]->final_pos[1];

            int16_t s1 = (int16_t)current_poly->vertices[vert]->texcoords[0],
                    s2 = (int16_t)current_poly->vertices[vert2]->texcoords[0],
                    t1 = (int16_t)current_poly->vertices[vert]->texcoords[1],
                    t2 = (int16_t)current_poly->vertices[vert2]->texcoords[1];

            int32_t z1 = current_poly->final_z[vert],
                    z2 = current_poly->final_z[vert2];
            int32_t w1 = current_poly->final_w[vert],
                    w2 = current_poly->final_w[vert2];

            uint32_t r1 = current_poly->vertices[vert]->final_colors[0],
                     g1 = current_poly->vertices[vert]->final_colors[1],
                     b1 = current_poly->vertices[vert]->final_colors[2],
                     r2 = current_poly->vertices[vert2]->final_colors[0],
                     g2 = current_poly->vertices[vert2]->final_colors[1],
                     b2 = current_poly->vertices[vert2]->final_colors[2];

            //Transpose steep lines (lines with a positive slope greater than one)
            bool steep = abs(y2 - y1) > abs(x2 - x1);
            if (steep)
            {
                swap(x1, y1);
                swap(x2, y2);
            }

            //Draw lines left-to-right
            if (x1 > x2)
            {
                swap(x1, x2);
                swap(y1, y2);
                swap(r1, r2);
                swap(g1, g2);
                swap(b1, b2);
                swap(z1, z2);
                swap(w1, w2);
                swap(s1, s2);
                swap(t1, t2);
            }

            int dx = x2 - x1;
            int dy = abs(y2 - y1);

            int error = dx / 2;
            int y_step = (y1 < y2) ? 1 : -1;

            int y = y1;

            int left_pixel = -1;
            int right_pixel = -1;
            for (int x = x1; x <= x2; x++)
            {
                if (steep)
                {
                    if (line == x)
                    {
                        if (y < left_x)
                        {
                            left_x = y;
                            left_pixel = x;
                        }
                        if (y > right_x)
                        {
                            right_x = y;
                            right_pixel = x;
                        }
                    }
                }
                else
                {
                    if (line == y)
                    {
                        if (x < left_x)
                        {
                            left_x = x;
                            left_pixel = x;
                        }
                        if (x > right_x)
                        {
                            right_x = x;
                            right_pixel = x;
                        }
                    }
                }
                error -= dy;
                if (error < 0)
                {
                    y += y_step;
                    error += dx;
                }
            }

            if (left_pixel >= 0)
            {
                uint64_t line_len = (x2 - x1);
                uint64_t left_pos = left_pixel - x1;
                left_r = interpolate(left_pos, line_len, r1, r2, w1, w2);
                left_g = interpolate(left_pos, line_len, g1, g2, w1, w2);
                left_b = interpolate(left_pos, line_len, b1, b2, w1, w2);
                left_z = interpolate(left_pos, line_len, z1, z2, w1, w2);
                left_w = (int32_t)interpolate(left_pos, line_len, w1, w2, w1, w2);
                left_s = (int16_t)interpolate(left_pos, line_len, s1, s2, w1, w2);
                left_t = (int16_t)interpolate(left_pos, line_len, t1, t2, w1, w2);
            }

            if (right_pixel >= 0)
            {
                int line_len = (x2 - x1);
                int right_pos = right_pixel - x1;
                right_r = interpolate(right_pos, line_len, r1, r2, w1, w2);
                right_g = interpolate(right_pos, line_len, g1, g2, w1, w2);
                right_b = interpolate(right_pos, line_len, b1, b2, w1, w2);
                right_z = interpolate(right_pos, line_len, z1, z2, w1, w2);
                right_w = (int32_t)interpolate(right_pos, line_len, w1, w2, w1, w2);
                right_s = (int16_t)interpolate(right_pos, line_len, s1, s2, w1, w2);
                right_t = (int16_t)interpolate(right_pos, line_len, t1, t2, w1, w2);
            }
        }

        if (right_x >= PIXELS_PER_LINE)
            right_x = PIXELS_PER_LINE - 1;
        if (left_x < 0)
            left_x = 0;

        int line_len = right_x - left_x;

        //Calculate texture stuff in advance
        TEXIMAGE_PARAM_REG texparams = current_poly->texparams;
        bool texture_mapping = DISP3DCNT.texture_mapping && texparams.format;
        int tex_width = 8 << texparams.s_size;
        int tex_height = 8 << texparams.t_size;
        uint32_t tex_VRAM_offset = texparams.VRAM_offset * 8;

        //Fill the polygon
        for (int x = left_x; x <= right_x; x++)
        {
            //Check if stencil buffer is set
            if (current_poly->shadow_poly && !stencil_buffer[x + (PIXELS_PER_LINE * (line & 0x1))])
                continue;

            //Depth test
            int pix_pos = x - left_x;
            uint32_t pix_z = interpolate(pix_pos, line_len, left_z, right_z, left_w, right_w);

            if (current_poly->attributes.depth_test_equal)
            {
                uint32_t low_z = z_buffer[line][x] - 0x200;
                uint32_t high_z = z_buffer[line][x] + 0x200;
                if (pix_z < low_z || pix_z > high_z)
                    continue;
            }
            else
            {
                if (pix_z > z_buffer[line][x])
                    continue;
            }
            uint32_t final_color = 0;
            uint32_t vr = 0, vg = 0, vb = 0, va = 0;
            uint16_t tr = 0x3E, tg = 0x3E, tb = 0x3E, ta = 0x1F;

            //Handle wireframe drawing
            if (current_poly->attributes.alpha == 0)
            {
                if (x == left_x || x == right_x)
                    va = 0x1F;
                else if (line == current_poly->top_y || line == current_poly->bottom_y)
                    va = 0x1F;
                else
                    continue;
            }
            else
            {
                //Normal alpha rules
                va = current_poly->attributes.alpha;

            }

            vr = interpolate(pix_pos, line_len, left_r, right_r, left_w, right_w) >> 3;
            vg = interpolate(pix_pos, line_len, left_g, right_g, left_w, right_w) >> 3;
            vb = interpolate(pix_pos, line_len, left_b, right_b, left_w, right_w) >> 3;

            uint32_t r, g, b;
            if (texture_mapping)
            {
                int16_t s, t;
                s = (int16_t)interpolate(pix_pos, line_len, left_s, right_s, left_w, right_w) >> 4;
                t = (int16_t)interpolate(pix_pos, line_len, left_t, right_t, left_w, right_w) >> 4;
                if (!texparams.repeat_s)
                {
                    //Clamp
                    if (s < 0)
                        s = 0;
                    if (s >= tex_width)
                        s = tex_width - 1;
                }
                else
                {
                    //Repeat
                    if (texparams.flip_s && (s & tex_width))
                        s = (tex_width - 1) - (s & (tex_width - 1));
                    else
                        s &= tex_width - 1;
                }

                if (!texparams.repeat_t)
                {
                    if (t < 0)
                        t = 0;
                    if (t >= tex_height)
                        t = tex_height - 1;
                }
                else
                {
                    if (texparams.flip_t && (t & tex_height))
                        t = (tex_height - 1) - (t & (tex_height - 1));
                    else
                        t &= tex_height - 1;
                }
                //printf("\n(%d, %d): ($%04X, $%04X)", x, line, s, t);
                switch (texparams.format)
                {
                    case 1: //A3I5
                    {
                        uint32_t texel_addr = s;
                        texel_addr += t * tex_width;
                        texel_addr += tex_VRAM_offset;
                        uint8_t data = gpu->read_teximage<uint8_t>(texel_addr);
                        int color_index = data & 0x1F;
                        int alpha_index = data >> 5;
                        ta = (alpha_index << 2) + (alpha_index >> 1);

                        if (color_index || !texparams.color0_transparent)
                        {
                            uint32_t pal_addr = color_index * 2;
                            pal_addr += current_poly->palette_base * 0x10;
                            uint16_t pal_color = gpu->read_texpal<uint16_t>(pal_addr);
                            tr = (pal_color & 0x1F) << 1;
                            tg = ((pal_color >> 5) & 0x1F) << 1;
                            tb = ((pal_color >> 10) & 0x1F) << 1;
                        }
                        else
                            ta = 0;
                    }
                        break;
                    case 2: //4 color palette
                    {
                        uint32_t texel_addr = s;
                        texel_addr += t * tex_width;
                        texel_addr /= 4;
                        texel_addr += tex_VRAM_offset;
                        uint8_t data = gpu->read_teximage<uint8_t>(texel_addr);
                        data = (data >> ((s & 0x3) * 2)) & 0x3;

                        if (data || !texparams.color0_transparent)
                        {
                            uint32_t pal_addr = data * 2;
                            pal_addr += current_poly->palette_base * 0x8;
                            uint16_t pal_color = gpu->read_texpal<uint16_t>(pal_addr);
                            tr = (pal_color & 0x1F) << 1;
                            tg = ((pal_color >> 5) & 0x1F) << 1;
                            tb = ((pal_color >> 10) & 0x1F) << 1;
                        }
                        else
                            ta = 0;
                    }
                        break;
                    case 3: //16 color palette
                    {
                        uint32_t texel_addr = s;
                        texel_addr += t * tex_width;
                        texel_addr /= 2;
                        texel_addr += tex_VRAM_offset;
                        uint8_t data = gpu->read_teximage<uint8_t>(texel_addr);
                        if (s & 0x1)
                            data >>= 4;
                        else
                            data &= 0xF;

                        if (data || !texparams.color0_transparent)
                        {
                            uint32_t pal_addr = data * 2;
                            pal_addr += current_poly->palette_base * 0x10;
                            uint16_t pal_color = gpu->read_texpal<uint16_t>(pal_addr);
                            tr = (pal_color & 0x1F) << 1;
                            tg = ((pal_color >> 5) & 0x1F) << 1;
                            tb = ((pal_color >> 10) & 0x1F) << 1;
                        }
                        else
                            ta = 0;
                    }
                        break;
                    case 4: //256 color palette
                    {
                        uint32_t texel_addr = s;
                        texel_addr += t * tex_width;
                        texel_addr += tex_VRAM_offset;
                        uint8_t data = gpu->read_teximage<uint8_t>(texel_addr);

                        if (data || !texparams.color0_transparent)
                        {
                            uint32_t pal_addr = data * 2;
                            pal_addr += current_poly->palette_base * 0x10;
                            uint16_t pal_color = gpu->read_texpal<uint16_t>(pal_addr);
                            tr = (pal_color & 0x1F) << 1;
                            tg = ((pal_color >> 5) & 0x1F) << 1;
                            tb = ((pal_color >> 10) & 0x1F) << 1;
                        }
                        else
                            ta = 0;
                    }
                        break;
                    case 5: //Compressed 4x4
                    {
                        uint32_t texel_addr = s & 0x3FC;
                        texel_addr += (t & 0x3FC) * (tex_width >> 2);
                        texel_addr += tex_VRAM_offset;
                        texel_addr += (t & 0x3);

                        uint32_t slot1_addr = 0x20000 + ((texel_addr >> 1) & 0xFFFE);
                        if (texel_addr >= 0x40000)
                            slot1_addr += 0x10000;

                        uint8_t data = gpu->read_teximage<uint8_t>(texel_addr);
                        data >>= 2 * (s & 0x3);
                        data &= 0x3;
                        uint16_t palette_data = gpu->read_teximage<uint16_t>(slot1_addr);
                        uint32_t palette_offset = (palette_data & 0x3FFF) << 2;

                        uint32_t palette_base = current_poly->palette_base * 0x10;

                        uint16_t color = 0;
                        switch (data)
                        {
                            case 0:
                                color = gpu->read_texpal<uint16_t>(palette_base + palette_offset);
                                break;
                            case 1:
                                color = gpu->read_texpal<uint16_t>(palette_base + palette_offset + 2);
                                break;
                            case 2:
                                if ((palette_data >> 14) == 1)
                                {
                                    uint16_t color0 = gpu->read_texpal<uint16_t>(palette_base + palette_offset);
                                    uint16_t color1 = gpu->read_texpal<uint16_t>(palette_base + palette_offset + 2);

                                    int r0 = color0 & 0x1F, r1 = color1 & 0x1F;
                                    int g0 = (color0 >> 5) & 0x1F, g1 = (color1 >> 5) & 0x1F;
                                    int b0 = (color0 >> 10) & 0x1F, b1 = (color1 >> 10) & 0x1F;

                                    int r = (r0 + r1) >> 1;
                                    int g = (g0 + g1) >> 1;
                                    int b = (b0 + b1) >> 1;

                                    color = r | (g << 5) | (b << 10);
                                }
                                else if ((palette_data >> 14) == 3)
                                {
                                    uint16_t color0 = gpu->read_texpal<uint16_t>(palette_base + palette_offset);
                                    uint16_t color1 = gpu->read_texpal<uint16_t>(palette_base + palette_offset + 2);

                                    int r0 = color0 & 0x1F, r1 = color1 & 0x1F;
                                    int g0 = (color0 >> 5) & 0x1F, g1 = (color1 >> 5) & 0x1F;
                                    int b0 = (color0 >> 10) & 0x1F, b1 = (color1 >> 10) & 0x1F;

                                    int r = (r0 * 5 + r1 * 3) >> 3;
                                    int g = (g0 * 5 + g1 * 3) >> 3;
                                    int b = (b0 * 5 + b1 * 3) >> 3;

                                    color = r | (g << 5) | (b << 10);
                                }
                                else
                                    color = gpu->read_texpal<uint16_t>(palette_base + palette_offset + 4);
                                break;
                            case 3:
                                if ((palette_data >> 14) == 2)
                                    color = gpu->read_texpal<uint16_t>(palette_base + palette_offset + 6);
                                else if ((palette_data >> 14) == 3)
                                {
                                    uint16_t color0 = gpu->read_texpal<uint16_t>(palette_base + palette_offset);
                                    uint16_t color1 = gpu->read_texpal<uint16_t>(palette_base + palette_offset + 2);

                                    int r0 = color0 & 0x1F, r1 = color1 & 0x1F;
                                    int g0 = (color0 >> 5) & 0x1F, g1 = (color1 >> 5) & 0x1F;
                                    int b0 = (color0 >> 10) & 0x1F, b1 = (color1 >> 10) & 0x1F;

                                    int r = (r0 * 3 + r1 * 5) >> 3;
                                    int g = (g0 * 3 + g1 * 5) >> 3;
                                    int b = (b0 * 3 + b1 * 5) >> 3;

                                    color = r | (g << 5) | (b << 10);
                                }
                                else
                                {
                                    color = 0;
                                    ta = 0;
                                }
                                break;
                            default:
                                printf("\nUnrecognized 4x4 texel data type %d", data);
                                throw "[3D] Unrecognized 4x4 texel data type";
                        }

                        tr = (color & 0x1F) << 1;
                        tg = ((color >> 5) & 0x1F) << 1;
                        tb = ((color >> 10) & 0x1F) << 1;
                    }
                        break;
                    case 6: //A5I3
                    {
                        uint32_t texel_addr = s;
                        texel_addr += t * tex_width;
                        texel_addr += tex_VRAM_offset;
                        uint8_t data = gpu->read_teximage<uint8_t>(texel_addr);
                        int color_index = data & 0x7;
                        ta = data >> 3;

                        if (color_index || !texparams.color0_transparent)
                        {
                            uint32_t pal_addr = color_index * 2;
                            pal_addr += current_poly->palette_base * 0x10;
                            uint16_t pal_color = gpu->read_texpal<uint16_t>(pal_addr);
                            tr = (pal_color & 0x1F) << 1;
                            tg = ((pal_color >> 5) & 0x1F) << 1;
                            tb = ((pal_color >> 10) & 0x1F) << 1;
                        }
                        else
                            ta = 0;
                    }
                        break;
                    case 7: //Direct color
                    {
                        uint32_t texel_addr = s;
                        texel_addr += t * tex_width;
                        texel_addr *= 2;
                        texel_addr += tex_VRAM_offset;
                        uint16_t data = gpu->read_teximage<uint16_t>(texel_addr);
                        if (data & (1 << 15))
                        {
                            tr = (data & 0x1F) << 1;
                            tg = ((data >> 5) & 0x1F) << 1;
                            tb = ((data >> 10) & 0x1F) << 1;
                        }
                        else
                            ta = 0;
                    }
                        break;
                    default:
                        printf("\nUnrecognized texture format %d", current_poly->texparams.format);
                }
            }

            tr += !(!tr);
            tg += !(!tg);
            tb += !(!tb);

            /*if (tr)
                tr++;
            if (tg)
                tg++;
            if (tb)
                tb++;*/

            int alpha;

            switch (current_poly->attributes.polygon_mode)
            {
                case 0:
                case 3:
                    r = (((tr + 1) * (vr + 1) - 1) / 64);
                    g = (((tg + 1) * (vg + 1) - 1) / 64);
                    b = (((tb + 1) * (vb + 1) - 1) / 64);
                    alpha = (((ta + 1) * (va + 1) - 1)) / 32;
                    break;
                case 2:
                    if (DISP3DCNT.highlight_shading)
                    {
                        vg = vr;
                        vb = vr;
                    }
                    else
                    {
                        uint16_t toon_color = TOON_TABLE[vr >> 1];

                        vr = (toon_color & 0x1F) << 1;
                        vg = ((toon_color >> 5) & 0x1F) << 1;
                        vb = ((toon_color >> 10) & 0x1F) << 1;

                        if (vr)
                            vr++;
                        if (vg)
                            vg++;
                        if (vb)
                            vb++;
                    }
                    r = (((tr + 1) * (vr + 1) - 1) / 64);
                    g = (((tg + 1) * (vg + 1) - 1) / 64);
                    b = (((tb + 1) * (vb + 1) - 1) / 64);
                    alpha = (((ta + 1) * (va + 1) - 1)) / 32;
                    break;
                default:
                    printf("\nUnrecognized polygon rendering mode %d", current_poly->attributes.polygon_mode);
                    throw "[3D] Unrecognized polygon rendering mode";
            }

            if (!alpha)
                continue;

            if (!current_poly->translucent || current_poly->attributes.set_new_trans_depth)
                z_buffer[line][x] = pix_z;

            if (current_poly->translucent)
            {
                if (DISP3DCNT.alpha_blending)
                {
                    if (current_poly->shadow_poly)
                    {
                        if (attr_buffer[x].translucent)
                        {
                            if (attr_buffer[x].trans_id == current_poly->attributes.id)
                                continue;
                        }
                        else
                        {
                            if (attr_buffer[x].opaque_id == current_poly->attributes.id)
                                continue;
                        }
                    }
                    else
                    {
                        if (attr_buffer[x].translucent && attr_buffer[x].trans_id == current_poly->attributes.id)
                            continue;
                    }
                    int pa = (framebuffer[x] >> 24) & 0x1F;
                    int pr = (framebuffer[x] >> 16) & 0x3F;
                    int pg = (framebuffer[x] >> 8) & 0x3F;
                    int pb = framebuffer[x] & 0x3F;

                    if (pa)
                    {
                        r = (((alpha + 1) * r) + (31 - alpha) * pr) / 32;
                        g = (((alpha + 1) * g) + (31 - alpha) * pg) / 32;
                        b = (((alpha + 1) * b) + (31 - alpha) * pb) / 32;
                        alpha = max(alpha, pa);
                    }
                }
                attr_buffer[x].translucent = true;
                attr_buffer[x].trans_id = current_poly->attributes.id;
            }
            else
            {
                attr_buffer[x].translucent = false;
                attr_buffer[x].edge = false;
                if (x == left_x || x == right_x || line == current_poly->top_y || line == current_poly->bottom_y)
                {
                    if (attr_buffer[x].opaque_id != current_poly->attributes.id)
                        attr_buffer[x].edge = true;
                }

                attr_buffer[x].opaque_id = current_poly->attributes.id;

            }

            if (alpha > 0x1F)
                alpha = 0x1F;

            final_color |= alpha << 24;
            final_color |= r << 16;
            final_color |= g << 8;
            final_color |= b;

            framebuffer[x] = 0x80000000 + final_color;
            attr_buffer[x].fog = current_poly->attributes.fog_enable;
        }
    }
    /*if (DISP3DCNT.edge_marking)
    {
        for (int i = 0; i < PIXELS_PER_LINE; i++)
        {
            if (attr_buffer[i].edge)
            {
                bool mark_edge = false;
                if (i > 0)
                {
                    if (attr_buffer[i].opaque_id != attr_buffer[i - 1].opaque_id)
                    {
                        if (z_buffer[line][i] < z_buffer[line][i - 1])
                            mark_edge = true;
                    }
                }
                if (i < PIXELS_PER_LINE - 1)
                {
                    if (attr_buffer[i].opaque_id != attr_buffer[i + 1].opaque_id)
                    {
                        if (z_buffer[line][i] < z_buffer[line][i + 1])
                            mark_edge = true;
                    }
                }
                if (mark_edge)
                {
                    uint32_t new_color = 0x80000000;
                    framebuffer[i] = new_color;
                }
            }
        }
    }*/
    if (DISP3DCNT.fog_enable)
    {
        for (int i = 0; i < PIXELS_PER_LINE; i++)
        {
            if (attr_buffer[i].fog)
            {
                uint32_t z = z_buffer[line][i];
                uint8_t density;
                if (z < FOG_OFFSET)
                {
                    density = 0;
                }
                else
                {
                    //TODO: linearly interpolate density
                    z -= FOG_OFFSET;
                    int depth_boundary = (z >> 2);
                    depth_boundary <<= DISP3DCNT.fog_depth_shift;
                    depth_boundary >>= 17;
                    if (depth_boundary > 31)
                        depth_boundary = 31;
                    density = FOG_TABLE[depth_boundary];
                    density += (density == 0x7F); //0x7F gets treated as 0x80
                }
                if (!DISP3DCNT.fog_alpha_only)
                {
                    uint32_t new_color = 0x80000000;
                    int fog_r = (FOG_COLOR & 0x1F) << 1;
                    int fog_g = ((FOG_COLOR >> 5) & 0x1F) << 1;
                    int fog_b = ((FOG_COLOR >> 10) & 0x1F) << 1;
                    int fog_a = ((FOG_COLOR >> 16) & 0x1F);

                    if (fog_r)
                        fog_r++;
                    if (fog_g)
                        fog_g++;
                    if (fog_b)
                        fog_b++;
                    int poly_r = (framebuffer[i] >> 16) & 0x3F;
                    int poly_g = (framebuffer[i] >> 8) & 0x3F;
                    int poly_b = framebuffer[i] & 0x3F;
                    int poly_a = (framebuffer[i] >> 24) & 0x1F;

                    new_color |= ((fog_a * density + poly_a * (128 - density)) / 128) << 24;
                    new_color |= ((fog_r * density + poly_r * (128 - density)) / 128) << 16;
                    new_color |= ((fog_g * density + poly_g * (128 - density)) / 128) << 8;
                    new_color |= ((fog_b * density + poly_b * (128 - density)) / 128);
                    framebuffer[i] = new_color;
                }
            }
        }
    }
}

void GPU_3D::render_shadow_mask(Polygon *current_poly)
{
    //This is a copy-paste of the normal render function
    //TODO: Move all this interpolation shit to a new function
    int line = gpu->get_VCOUNT();
    int stencil_offset = PIXELS_PER_LINE * (line & 0x1);
    if (!previous_shadow_mask)
        memset(stencil_buffer + stencil_offset, 0, PIXELS_PER_LINE);

    previous_shadow_mask = true;

    int left_x = 512, right_x = -512;
    uint32_t left_z, right_z;
    int32_t left_w, right_w;

    //Figure out the leftmost/rightmost points on the polygon on this scanline
    //Using a variation of Bresenham's line algorithm
    for (int vert = 0; vert < current_poly->vert_count; vert++)
    {
        int vert2 = (vert + 1) % current_poly->vert_count;
        int x1 = current_poly->vertices[vert]->final_pos[0],
            y1 = current_poly->vertices[vert]->final_pos[1],
            x2 = current_poly->vertices[vert2]->final_pos[0],
            y2 = current_poly->vertices[vert2]->final_pos[1];

        int32_t z1 = current_poly->final_z[vert],
                z2 = current_poly->final_z[vert2];
        int32_t w1 = current_poly->final_w[vert],
                w2 = current_poly->final_w[vert2];

        //Transpose steep lines (lines with a positive slope greater than one)
        bool steep = abs(y2 - y1) > abs(x2 - x1);
        if (steep)
        {
            swap(x1, y1);
            swap(x2, y2);
        }

        //Draw lines left-to-right
        if (x1 > x2)
        {
            swap(x1, x2);
            swap(y1, y2);
            swap(z1, z2);
            swap(w1, w2);
        }

        int dx = x2 - x1;
        int dy = abs(y2 - y1);

        int error = dx / 2;
        int y_step = (y1 < y2) ? 1 : -1;

        int y = y1;

        int left_pixel = -1;
        int right_pixel = -1;
        for (int x = x1; x <= x2; x++)
        {
            if (steep)
            {
                if (line == x)
                {
                    if (y < left_x)
                    {
                        left_x = y;
                        left_pixel = x;
                    }
                    if (y > right_x)
                    {
                        right_x = y;
                        right_pixel = x;
                    }
                }
            }
            else
            {
                if (line == y)
                {
                    if (x < left_x)
                    {
                        left_x = x;
                        left_pixel = x;
                    }
                    if (x > right_x)
                    {
                        right_x = x;
                        right_pixel = x;
                    }
                }
            }
            error -= dy;
            if (error < 0)
            {
                y += y_step;
                error += dx;
            }
        }

        if (left_pixel >= 0)
        {
            uint64_t line_len = (x2 - x1);
            uint64_t left_pos = left_pixel - x1;
            left_z = interpolate(left_pos, line_len, z1, z2, w1, w2);
            left_w = (int32_t)interpolate(left_pos, line_len, w1, w2, w1, w2);
        }

        if (right_pixel >= 0)
        {
            int line_len = (x2 - x1);
            int right_pos = right_pixel - x1;
            right_z = interpolate(right_pos, line_len, z1, z2, w1, w2);
            right_w = (int32_t)interpolate(right_pos, line_len, w1, w2, w1, w2);
        }
    }

    if (right_x >= PIXELS_PER_LINE)
        right_x = PIXELS_PER_LINE - 1;
    if (left_x < 0)
        left_x = 0;

    int line_len = right_x - left_x;
    for (int x = left_x; x <= right_x; x++)
    {
        //Stencil buffer is only set when the depth test fails
        int pix_pos = x - left_x;
        uint32_t pix_z = interpolate(pix_pos, line_len, left_z, right_z, left_w, right_w);

        if (current_poly->attributes.depth_test_equal)
        {
            uint32_t low_z = z_buffer[line][x] - 0x200;
            uint32_t high_z = z_buffer[line][x] + 0x200;
            if (pix_z < low_z || pix_z > high_z)
                stencil_buffer[x + stencil_offset] = true;
        }
        else
        {
            if (pix_z >= z_buffer[line][x])
                stencil_buffer[x + stencil_offset] = true;
        }
    }
}

void GPU_3D::run(uint64_t cycles_to_run)
{
    if (swap_buffers)
        return;
    if (cycles <= 0 && !GXPIPE.size())
    {
        cycles = 0;
        return;
    }
    cycles -= cycles_to_run;
    while (cycles <= 0 && GXPIPE.size())
        exec_command();
}

void GPU_3D::check_FIFO_DMA()
{
    if (GXFIFO.size() < 128)
        e->GXFIFO_DMA_request();
}

void GPU_3D::check_FIFO_IRQ()
{
    switch (GXSTAT.GXFIFO_irq_stat)
    {
        case 1:
            if (GXFIFO.size() < 128)
                e->request_interrupt9(INTERRUPT::GEOMETRY_FIFO);
            break;
        case 2:
            if (GXFIFO.size() == 0)
                e->request_interrupt9(INTERRUPT::GEOMETRY_FIFO);
            break;
        default:
            //Never send IRQ requests
            break;
    }
}

void GPU_3D::write_GXFIFO(uint32_t word)
{
    //printf("\nWrite GXFIFO: $%08X", word);
    if (cmd_count == 0)
    {
        cmd_count = 4;
        current_cmd = word;
        param_count = 0;
        total_params = cmd_param_amounts[word & 0xFF];

        if (total_params > 0)
            return;
    }
    else
        param_count++;

    while (true)
    {
        if ((current_cmd & 0xFF) || (cmd_count == 4 && current_cmd == 0))
        {
            GX_Command cmd;
            cmd.command = current_cmd & 0xFF;
            cmd.param = word;
            write_command(cmd);
        }
        if (param_count >= total_params)
        {
            current_cmd >>= 8;
            cmd_count--;
            if (!cmd_count)
                break;

            param_count = 0;
            total_params = cmd_param_amounts[current_cmd & 0xFF];
        }
        if (param_count < total_params)
            break;
    }
}

void GPU_3D::write_FIFO_direct(uint32_t address, uint32_t word)
{
    //printf("\nWrite FIFO direct: $%08X, $%08X", address, word);
    GX_Command cmd;
    cmd.command = (address >> 2) & 0x7F;
    cmd.param = word;
    write_command(cmd);
}

void GPU_3D::get_identity_mtx(MTX &mtx)
{
    mtx.set(IDENTITY);
}

GX_Command GPU_3D::read_command()
{
    GX_Command cmd = GXPIPE.front();
    GXPIPE.pop();

    //Refill the pipe if it is at least half-empty
    if (GXPIPE.size() < 3)
    {
        if (GXFIFO.size())
        {
            GXPIPE.push(GXFIFO.front());
            GXFIFO.pop();
        }
        if (GXFIFO.size())
        {
            GXPIPE.push(GXFIFO.front());
            GXFIFO.pop();
        }

        check_FIFO_DMA();
        check_FIFO_IRQ();
    }

    //Check if the next command is a BOX/POS/VEC test or matrix stack operation
    //And update relevant flags
    if (GXPIPE.size())
    {
        GX_Command next_cmd = GXPIPE.front();
        GXSTAT.mtx_stack_busy = next_cmd.command == 0x11 || next_cmd.command == 0x12;
        GXSTAT.box_pos_vec_busy = next_cmd.command == 0x70 || next_cmd.command == 0x71 || next_cmd.command == 0x72;
    }
    else
    {
        GXSTAT.mtx_stack_busy = false;
        GXSTAT.box_pos_vec_busy = false;
    }

    return cmd;
}

void GPU_3D::write_command(GX_Command &cmd)
{
    if (!GXFIFO.size() && GXPIPE.size() < 4)
        GXPIPE.push(cmd);
    else
    {
        if (GXFIFO.size() >= 256)
        {
            //printf("\nGXFIFO full!");
            while (GXFIFO.size() >= 256)
                exec_command();
        }
        GXFIFO.push(cmd);
    }
}

#ifndef GX_DEBUG
#define printf(fmt, ...)(0)
#endif
void GPU_3D::exec_command()
{
    GX_Command cmd = read_command();

    cmd_params[cmd_param_count] = cmd.param;
    cmd_param_count++;

    if (cmd_param_count >= cmd_param_amounts[cmd.command])
    {
        cycles += cmd_cycle_amounts[cmd.command];

        GXSTAT.geo_busy = (cycles > 0 && GXPIPE.size());
        switch (cmd.command)
        {
            case 0x00:
                break;
            case 0x10:
                printf("\nMTX_MODE: $%08X", cmd_params[0]);
                MTX_MODE = cmd_params[0] & 0x3;
                break;
            case 0x11:
                printf("\nMTX_PUSH");
                MTX_PUSH();
                break;
            case 0x12:
                printf("\nMTX_POP: $%08X", cmd_params[0]);
            {
                int8_t offset = ((int8_t)((cmd_params[0] & 0x3F) << 2)) >> 2;
                printf("\nOffset: %d", offset);
                if (MTX_MODE != 3)
                    clip_dirty = true;
                switch (MTX_MODE)
                {
                    case 0:
                        if (projection_sp)
                        {
                            projection_mtx.set(projection_stack);
                            projection_sp--;
                        }
                        else
                        {
                            printf("\nMTX_POP overflow!");
                            GXSTAT.mtx_overflow = true;
                        }
                        break;
                    case 1:
                    case 2:
                        modelview_sp -= offset;
                        if (modelview_sp >= 0x1F)
                        {
                            printf("\nMTX_POP overflow!");
                            GXSTAT.mtx_overflow = true;
                        }
                        else
                        {
                            modelview_mtx.set(modelview_stack[modelview_sp & 0x1F]);
                            vector_mtx.set(vector_stack[modelview_sp & 0x1F]);
                        }
                        modelview_sp &= 0x3F;
                        break;
                    case 3:
                        texture_mtx.set(texture_stack);
                        break;
                    default:
                        printf("\nUnrecognized MTX_MODE %d for MTX_POP", MTX_MODE);
                }
            }
                break;
            case 0x13:
                printf("\nMTX_STORE");
            {
                uint8_t offset = cmd_params[0] & 0x1F;
                switch (MTX_MODE)
                {
                    case 0:
                        projection_stack.set(projection_mtx);
                        break;
                    case 1:
                    case 2:
                        if (offset < 31)
                        {
                            modelview_stack[offset].set(modelview_mtx);
                            vector_stack[offset].set(vector_mtx);
                        }
                        else
                            GXSTAT.mtx_overflow = true;
                        break;
                    case 3:
                        texture_stack.set(texture_mtx);
                        break;
                    default:
                        printf("\nUnrecognized MTX_MODE %d for MTX_STORE", MTX_MODE);
                }
            }
                break;
            case 0x14:
                printf("\nMTX_RESTORE $%02X", cmd_params[0] & 0xFF);
                if (MTX_MODE != 3)
                    clip_dirty = true;
                switch (MTX_MODE)
                {
                    case 0:
                        projection_mtx.set(projection_stack);
                        break;
                    case 1:
                    case 2:
                    {
                        uint8_t offset = cmd_params[0] & 0x1F;
                        if (offset < 31)
                        {
                            modelview_mtx.set(modelview_stack[offset]);
                            vector_mtx.set(vector_stack[offset]);
                        }
                        else
                            GXSTAT.mtx_overflow = true;
                    }
                        break;
                    case 3:
                        texture_mtx.set(texture_stack);
                        break;
                    default:
                        printf("\nUnrecognized MTX_MODE %d for MTX_RESTORE", MTX_MODE);
                }
                break;
            case 0x15:
                printf("\nMTX_IDENTITY");
                if (MTX_MODE != 3)
                    clip_dirty = true;
                MTX_IDENTITY();
                break;
            case 0x16:
                printf("\nMTX_LOAD_4x4");
            {
                MTX* current_mtx = nullptr;
                if (MTX_MODE != 3)
                    clip_dirty = true;
                switch (MTX_MODE)
                {
                    case 0:
                        current_mtx = &projection_mtx;
                        break;
                    case 1:
                        current_mtx = &modelview_mtx;
                        break;
                    case 2:
                        current_mtx = &modelview_mtx;
                        for (int i = 0; i < 4; i++)
                        {
                            for (int j = 0; j < 4; j++)
                                vector_mtx.m[i][j] = (int32_t)cmd_params[(i * 4) + j];
                        }
                        break;
                    case 3:
                        current_mtx = &texture_mtx;
                        break;
                    default:
                        printf("\nUnrecognized MTX_MODE %d in MTX_LOAD_4x4", MTX_MODE);
                }
                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++)
                        current_mtx->m[i][j] = (int32_t)cmd_params[(i * 4) + j];
                }
            }
                break;
            case 0x17:
                printf("\nMTX_LOAD_4x3");
                if (MTX_MODE != 3)
                    clip_dirty = true;
            {
                MTX* current_mtx = nullptr;
                switch (MTX_MODE)
                {
                    case 0:
                        current_mtx = &projection_mtx;
                        break;
                    case 1:
                        current_mtx = &modelview_mtx;
                        break;
                    case 2:
                        current_mtx = &modelview_mtx;
                        vector_mtx.m[0][0] = (int32_t)cmd_params[0];
                        vector_mtx.m[0][1] = (int32_t)cmd_params[1];
                        vector_mtx.m[0][2] = (int32_t)cmd_params[2];
                        vector_mtx.m[1][0] = (int32_t)cmd_params[3];
                        vector_mtx.m[1][1] = (int32_t)cmd_params[4];
                        vector_mtx.m[1][2] = (int32_t)cmd_params[5];
                        vector_mtx.m[2][0] = (int32_t)cmd_params[6];
                        vector_mtx.m[2][1] = (int32_t)cmd_params[7];
                        vector_mtx.m[2][2] = (int32_t)cmd_params[8];
                        vector_mtx.m[3][0] = (int32_t)cmd_params[9];
                        vector_mtx.m[3][1] = (int32_t)cmd_params[10];
                        vector_mtx.m[3][2] = (int32_t)cmd_params[11];
                        break;
                    case 3:
                        current_mtx = &texture_mtx;
                        break;
                    default:
                        printf("\nUnrecognized MTX_MODE %d for MTX_LOAD_4x3", MTX_MODE);
                }
                current_mtx->m[0][0] = (int32_t)cmd_params[0];
                current_mtx->m[0][1] = (int32_t)cmd_params[1];
                current_mtx->m[0][2] = (int32_t)cmd_params[2];
                current_mtx->m[1][0] = (int32_t)cmd_params[3];
                current_mtx->m[1][1] = (int32_t)cmd_params[4];
                current_mtx->m[1][2] = (int32_t)cmd_params[5];
                current_mtx->m[2][0] = (int32_t)cmd_params[6];
                current_mtx->m[2][1] = (int32_t)cmd_params[7];
                current_mtx->m[2][2] = (int32_t)cmd_params[8];
                current_mtx->m[3][0] = (int32_t)cmd_params[9];
                current_mtx->m[3][1] = (int32_t)cmd_params[10];
                current_mtx->m[3][2] = (int32_t)cmd_params[11];
                if (geo_poly_count == 288 && MTX_MODE == 2)
                {
                    printf("\nLOAD_4X3 MODELVIEW");
                    for (int i = 0; i < cmd_param_count; i++)
                    {
                        printf("$%08X ", cmd_params[i]);
                    }
                }
            }
                break;
            case 0x18:
                printf("\nMTX_MULT_4x4");
            {
                int cmd_pointer = 0;
                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++, cmd_pointer++)
                        mult_params.m[i][j] = cmd_params[cmd_pointer];
                }
                MTX_MULT();
            }
                break;
            case 0x19:
                printf("\nMTX_MULT_4x3");
            {
                mult_params.m[0][0] = cmd_params[0];
                mult_params.m[0][1] = cmd_params[1];
                mult_params.m[0][2] = cmd_params[2];
                mult_params.m[1][0] = cmd_params[3];
                mult_params.m[1][1] = cmd_params[4];
                mult_params.m[1][2] = cmd_params[5];
                mult_params.m[2][0] = cmd_params[6];
                mult_params.m[2][1] = cmd_params[7];
                mult_params.m[2][2] = cmd_params[8];
                mult_params.m[3][0] = cmd_params[9];
                mult_params.m[3][1] = cmd_params[10];
                mult_params.m[3][2] = cmd_params[11];
                MTX_MULT();
            }
                break;
            case 0x1A:
                printf("\nMTX_MULT_3x3");
                mult_params.m[0][0] = cmd_params[0];
                mult_params.m[0][1] = cmd_params[1];
                mult_params.m[0][2] = cmd_params[2];
                mult_params.m[1][0] = cmd_params[3];
                mult_params.m[1][1] = cmd_params[4];
                mult_params.m[1][2] = cmd_params[5];
                mult_params.m[2][0] = cmd_params[6];
                mult_params.m[2][1] = cmd_params[7];
                mult_params.m[2][2] = cmd_params[8];
                MTX_MULT();
                break;
            case 0x1B:
                printf("\nMTX_SCALE: $%08X $%08X $%08X", cmd_params[0], cmd_params[1], cmd_params[2]);
                mult_params.m[0][0] = cmd_params[0];
                mult_params.m[1][1] = cmd_params[1];
                mult_params.m[2][2] = cmd_params[2];
                MTX_MULT(false);
                break;
            case 0x1C:
                printf("\nMTX_TRANS");
                mult_params.m[3][0] = cmd_params[0];
                mult_params.m[3][1] = cmd_params[1];
                mult_params.m[3][2] = cmd_params[2];
                MTX_MULT();
                break;
            case 0x20:
                //printf("\nCOLOR: $%08X", cmd_params[0]);
                current_color = cmd_params[0];
                break;
            case 0x21:
                NORMAL();
                break;
            case 0x22:
                //printf("\nTEXCOORD: $%08X", cmd_params[0]);
                raw_texcoords[0] = cmd_params[0] & 0xFFFF;
                raw_texcoords[1] = cmd_params[0] >> 16;
                if (TEXIMAGE_PARAM.transformation_mode == 1)
                {
                    current_texcoords[0] = (raw_texcoords[0] * texture_mtx.m[0][0] + raw_texcoords[1] * texture_mtx.m[1][0]
                            + texture_mtx.m[2][0] + texture_mtx.m[3][0]) >> 12;
                    current_texcoords[1] = (raw_texcoords[0] * texture_mtx.m[0][1] + raw_texcoords[1] * texture_mtx.m[1][1]
                            + texture_mtx.m[2][1] + texture_mtx.m[3][1]) >> 12;
                }
                else
                {
                    current_texcoords[0] = raw_texcoords[0];
                    current_texcoords[1] = raw_texcoords[1];
                }
                break;
            case 0x23:
                //printf("\nVTX_16: $%08X, $%08X", cmd_params[0], cmd_params[1]);
                current_vertex[0] = cmd_params[0] & 0xFFFF;
                current_vertex[1] = cmd_params[0] >> 16;
                current_vertex[2] = cmd_params[1] & 0xFFFF;
                add_vertex();
                break;
            case 0x24:
                //printf("\nVTX_10");
                current_vertex[0] = (cmd_params[0] & 0x000003FF) << 6;
                current_vertex[1] = (cmd_params[0] & 0x000FFC00) >> 4;
                current_vertex[2] = (cmd_params[0] & 0x3FF00000) >> 14;
                add_vertex();
                break;
            case 0x25:
                //printf("\nVTX_XY");
                current_vertex[0] = cmd_params[0] & 0xFFFF;
                current_vertex[1] = cmd_params[0] >> 16;
                add_vertex();
                break;
            case 0x26:
                //printf("\nVTX_XZ");
                current_vertex[0] = cmd_params[0] & 0xFFFF;
                current_vertex[2] = cmd_params[0] >> 16;
                add_vertex();
                break;
            case 0x27:
                //printf("\nVTX_YZ");
                current_vertex[1] = cmd_params[0] & 0xFFFF;
                current_vertex[2] = cmd_params[0] >> 16;
                add_vertex();
                break;
            case 0x28:
                //printf("\nVTX_DIFF");
                current_vertex[0] += (int16_t)((cmd_params[0] & 0x000003FF) << 6) >> 6;
                current_vertex[1] += (int16_t)((cmd_params[0] & 0x000FFC00) >> 4) >> 6;
                current_vertex[2] += (int16_t)((cmd_params[0] & 0x3FF00000) >> 14) >> 6;
                add_vertex();
                break;
            case 0x29:
                set_POLYGON_ATTR(cmd_params[0]);
                break;
            case 0x2A:
                set_TEXIMAGE_PARAM(cmd_params[0]);
                break;
            case 0x2B:
                printf("\nPLTT_BASE: $%08X", cmd_params[0]);
                PLTT_BASE = cmd_params[0] & 0x1FFF;
                break;
            case 0x30:
                printf("\nDIF_AMB");
                diffuse_color = cmd_params[0] & 0x7FFF;
                ambient_color = (cmd_params[0] >> 16) & 0x7FFF;
                if (cmd_params[0] & (1 << 15))
                    current_color = diffuse_color;
                break;
            case 0x31:
                printf("\nSPE_EMI");
                specular_color = cmd_params[0] & 0x7FFF;
                emission_color = (cmd_params[0] >> 16) & 0x7FFF;
                using_shine_table = cmd_params[0] & (1 << 15);
                break;
            case 0x32:
                printf("\nLIGHT_VECTOR");
            {
                int16_t light_vector[3];
                light_vector[0] = (int16_t)((cmd_params[0] & 0x3FF) << 6) >> 6;
                light_vector[1] = (int16_t)(((cmd_params[0] >> 10) & 0x3FF) << 6) >> 6;
                light_vector[2] = (int16_t)(((cmd_params[0] >> 20) & 0x3FF) << 6) >> 6;
                int index = cmd_params[0] >> 30;
                light_direction[index][0] = (light_vector[0] * vector_mtx.m[0][0] +
                        light_vector[1] * vector_mtx.m[1][0] +
                        light_vector[2] * vector_mtx.m[2][0]) >> 12;
                light_direction[index][1] = (light_vector[0] * vector_mtx.m[0][1] +
                        light_vector[1] * vector_mtx.m[1][1] +
                        light_vector[2] * vector_mtx.m[2][1]) >> 12;
                light_direction[index][2] = (light_vector[0] * vector_mtx.m[0][2] +
                        light_vector[1] * vector_mtx.m[1][2] +
                        light_vector[2] * vector_mtx.m[2][2]) >> 12;
            }
                break;
            case 0x33:
                printf("\nLIGHT_COLOR: $%08X", cmd_params[0]);
                light_color[cmd_params[0] >> 30] = cmd_params[0] & 0x7FFF;
                break;
            case 0x34:
                printf("\nSHININESS");
                for (int i = 0; i < 32; i++)
                {
                    int index = i * 4;
                    shine_table[index] = cmd_params[i] & 0xFF;
                    shine_table[index + 1] = (cmd_params[i] >> 8) & 0xFF;
                    shine_table[index + 2] = (cmd_params[i] >> 16) & 0xFF;
                    shine_table[index + 3] = cmd_params[i] >> 24;
                }
                break;
            case 0x40:
                //printf("\nBEGIN_VTXS");
                POLYGON_TYPE = cmd_params[0] & 0x3;
                current_poly_attr = POLYGON_ATTR;
                consecutive_polygons = 0;
                vertex_list_count = 0;
                last_poly_strip = nullptr;
                break;
            case 0x41:
                //printf("\nEND_VTXS");
                break;
            case 0x50:
                printf("\nSWAP_BUFFERS");
                SWAP_BUFFERS(cmd_params[0]);
                break;
            case 0x60:
                VIEWPORT(cmd_params[0]);
                break;
            case 0x70:
                printf("\nBOX_TEST");
                BOX_TEST();
                break;
            case 0x72:
                printf("\nVEC_TEST");
                VEC_TEST();
                break;
            default:
                printf("\nUnrecognized GXFIFO command $%02X", cmd.command);
                throw "[3D] Unrecognized GXFIFO command";
        }
        cmd_param_count = 0;
    }
}
#ifndef GX_DEBUG
#undef printf
#endif

void GPU_3D::add_mult_param(uint32_t word)
{
    mult_params.m[mult_params_index / 4][mult_params_index % 4] = word;
    mult_params_index++;
}

int GPU_3D::clip(Vertex *v_list, int v_len, int clip_start, bool add_attributes)
{
    v_len = clip_plane(0, v_list, v_len, clip_start, add_attributes);
    v_len = clip_plane(1, v_list, v_len, clip_start, add_attributes);
    v_len = clip_plane(2, v_list, v_len, clip_start, add_attributes);
    return v_len;
}

int GPU_3D::clip_plane(int plane, Vertex *v_list, int v_len, int clip_start, bool add_attributes)
{
    Vertex temp_v_list[10];
    int clip_index = clip_start;
    int prev_v, next_v;

    if (clip_start == 2)
    {
        temp_v_list[0] = v_list[0];
        temp_v_list[1] = v_list[1];
    }

    //Clip everything higher than w
    for (int i = clip_start; i < v_len; i++)
    {
        prev_v = i - 1;
        if (prev_v < 0)
            prev_v = v_len - 1;
        next_v = i + 1;
        if (next_v >= v_len)
            next_v = 0;

        Vertex v = v_list[i];
        if (v.coords[plane] > v.coords[3])
        {
            if (plane == 2 && !current_poly_attr.render_far_intersect)
            {
                return 0;
            }

            Vertex *vp = &v_list[prev_v];
            if (vp->coords[plane] <= vp->coords[3])
            {
                clip_vertex(plane, temp_v_list[clip_index], v, vp, 1, add_attributes);
                clip_index++;
            }

            Vertex *vn = &v_list[next_v];
            if (vn->coords[plane] <= vn->coords[3])
            {
                clip_vertex(plane, temp_v_list[clip_index], v, vn, 1, add_attributes);
                clip_index++;
            }
        }
        else
        {
            temp_v_list[clip_index] = v;
            clip_index++;
        }
    }

    v_len = clip_index;
    clip_index = clip_start;

    //Clip everything lower than -w
    for (int i = clip_start; i < v_len; i++)
    {
        prev_v = i - 1;
        if (prev_v < 0)
            prev_v = v_len - 1;
        next_v = i + 1;
        if (next_v >= v_len)
            next_v = 0;

        Vertex v = temp_v_list[i];
        if (v.coords[plane] < -v.coords[3])
        {
            Vertex *vp = &temp_v_list[prev_v];
            if (vp->coords[plane] >= -vp->coords[3])
            {
                clip_vertex(plane, v_list[clip_index], v, vp, -1, add_attributes);
                clip_index++;
            }

            Vertex *vn = &temp_v_list[next_v];
            if (vn->coords[plane] >= -vn->coords[3])
            {
                clip_vertex(plane, v_list[clip_index], v, vn, -1, add_attributes);
                clip_index++;
            }
        }
        else
        {
            v_list[clip_index] = v;
            clip_index++;
        }
    }

    return clip_index;
}

void GPU_3D::clip_vertex(int plane, Vertex &v_list, Vertex &v_out, Vertex *v_in, int side, bool add_attributes)
{
    //Copied this from melonDS
    int64_t factor_num = v_in->coords[3] - (side * v_in->coords[plane]);
    int32_t factor_den = factor_num - (v_out.coords[3] - (side * v_out.coords[plane]));

    if (factor_den == 0)
        throw "[3D] factor_den equals zero";

#define INTERPOLATE(var) v_list.var = (v_in->var + ((v_out.var - v_in->var) * factor_num) / factor_den);

    if (plane != 0) INTERPOLATE(coords[0]);
    if (plane != 1) INTERPOLATE(coords[1]);
    if (plane != 2) INTERPOLATE(coords[2]);

    INTERPOLATE(coords[3]);
    v_list.coords[plane] = side * v_list.coords[3];

    if (add_attributes)
    {
        INTERPOLATE(colors[0]);
        INTERPOLATE(colors[1]);
        INTERPOLATE(colors[2]);

        INTERPOLATE(texcoords[0]);
        INTERPOLATE(texcoords[1]);
    }
    v_list.clipped = true;
    #undef INTERPOLATE
}

void GPU_3D::add_polygon()
{
    if (vertex_list_count < 3 || vertex_list_count > 4)
        throw "[3D] add_polygon called with invalid vertex_list_count";
    //Cull front/back face polygons
    int64_t normal_x, normal_y, normal_z;

    Vertex *v0 = &vertex_list[0], *v1 = &vertex_list[1], *v2 = &vertex_list[2];

    //Culling code taken shamelessly from melonDS :P
    normal_x = ((int64_t)(v0->coords[1] - v1->coords[1]) * (v2->coords[3] - v1->coords[3]))
        - ((int64_t)(v0->coords[3]-v1->coords[3]) * (v2->coords[1]-v1->coords[1]));
    normal_y = ((int64_t)(v0->coords[3]-v1->coords[3]) * (v2->coords[0]-v1->coords[0]))
        - ((int64_t)(v0->coords[0]-v1->coords[0]) * (v2->coords[3]-v1->coords[3]));
    normal_z = ((int64_t)(v0->coords[0]-v1->coords[0]) * (v2->coords[1]-v1->coords[1]))
        - ((int64_t)(v0->coords[1]-v1->coords[1]) * (v2->coords[0]-v1->coords[0]));

    //TODO: check what real DS does. Maybe help StapleButter?
    while ((((normal_x>>31) ^ (normal_x>>63)) != 0) ||
           (((normal_y>>31) ^ (normal_y>>63)) != 0) ||
           (((normal_z>>31) ^ (normal_z>>63)) != 0))
    {
        normal_x >>= 4;
        normal_y >>= 4;
        normal_z >>= 4;
    }

    int64_t dot = (((int64_t)v1->coords[0] * normal_x) + ((int64_t)v1->coords[1] * normal_y)
            + ((int64_t)v1->coords[3] * normal_z));

    bool front_view = dot < 0;

    if (front_view)
    {
        if (!current_poly_attr.render_front)
        {
            last_poly_strip = nullptr;
            return;
        }
    }
    else if (dot > 0)
    {
        if (!current_poly_attr.render_back)
        {
            last_poly_strip = nullptr;
            return;
        }
    }

    int clip_start = 0;
    int clipped_count = vertex_list_count;
    Vertex clipped_list[10];
    Vertex* reused_list[2];

    int last_poly_verts = 0;
    //Attempt to attach vertices from last strip polygon to new one, if possible
    if (POLYGON_TYPE >= 2 && last_poly_strip)
    {
        int v0, v1;
        if (POLYGON_TYPE == 2)
        {
            if (consecutive_polygons & 0x1)
            {
                v0 = 2;
                v1 = 1;
            }
            else
            {
                v0 = 0;
                v1 = 2;
            }
            last_poly_verts = 3;
        }
        else
        {
            v0 = 3;
            v1 = 2;

            last_poly_verts = 4;
        }

        if (last_poly_strip->vert_count == last_poly_verts &&
            !last_poly_strip->vertices[v0]->clipped &&
            !last_poly_strip->vertices[v1]->clipped)
        {
            reused_list[0] = last_poly_strip->vertices[v0];
            reused_list[1] = last_poly_strip->vertices[v1];
            clipped_list[0] = *reused_list[0];
            clipped_list[1] = *reused_list[1];
            clip_start = 2;
        }
    }

    for (int i = clip_start; i < clipped_count; i++)
        clipped_list[i] = vertex_list[i];

    //Clip the fuck out of that polygon
    clipped_count = clip(clipped_list, clipped_count, clip_start, true);
    if (!clipped_count)
    {
        last_poly_strip = nullptr;
        return;
    }

    if (geo_poly_count >= 2048 || geo_vert_count + clipped_count >= 6144)
    {
        printf("\nPolygon/vertex RAM overflow!");
        //geo_poly_count++;
        last_poly_strip = nullptr;
        DISP3DCNT.RAM_overflow = true;
        return;
    }

    //Time to make a polygon!
    Polygon* poly = &geo_poly[geo_poly_count];
    poly->top_y = 1024;
    poly->bottom_y = 0;
    poly->vert_count = 0;
    if (last_poly_strip && clip_start > 0)
    {
        if (clipped_count == last_poly_verts)
        {
            poly->vertices[0] = reused_list[0];
            poly->vertices[1] = reused_list[1];
        }
        else
        {
            Vertex v0 = *reused_list[0];
            Vertex v1 = *reused_list[1];

            geo_vert[geo_vert_count] = v0;
            poly->vertices[0] = &geo_vert[geo_vert_count];
            geo_vert[geo_vert_count + 1] = v1;
            poly->vertices[1] = &geo_vert[geo_vert_count + 1];
            geo_vert_count += 2;
        }
        poly->vert_count += 2;
    }
    for (int i = clip_start; i < clipped_count; i++)
    {
        Vertex* v = &geo_vert[geo_vert_count];
        *v = clipped_list[i];
        poly->vertices[i] = v;
        poly->vert_count++;
        geo_vert_count++;

        //Viewport transformation
        int64_t xx = v->coords[0], yy = v->coords[1], ww = v->coords[3];
        int32_t final_x, final_y;

        if (ww == 0)
        {
            final_x = 0;
            final_y = 0;
            printf("\npoly%d ww equals 0??", i);
        }
        else
        {
            int width = (viewport.x2 - viewport.x1 + 1) & 0x1FF;
            int height = (viewport.y1 - viewport.y2 + 1) & 0xFF;
            int64_t screen_x = (((xx + ww) * width) / (ww << 1)) + viewport.x1;
            int64_t screen_y = (((-yy + ww) * height) / (ww << 1)) + viewport.y2;

            final_x = screen_x & 0x1FF;
            final_y = screen_y & 0xFF;
        }

        v->final_pos[0] = final_x;
        v->final_pos[1] = final_y;

        for (int c = 0; c < 3; c++)
        {
            v->final_colors[c] = v->colors[c] >> 12;
            if (v->final_colors[c])
            {
                v->final_colors[c] <<= 4;
                v->final_colors[c] += 0xF;
            }
        }
    }

    int w_size = 0;
    for (int i = 0; i < clipped_count; i++)
    {
        int y = poly->vertices[i]->final_pos[1];
        if (y < poly->top_y)
            poly->top_y = y;
        if (y > poly->bottom_y)
            poly->bottom_y = y;

        uint32_t w = (uint32_t)poly->vertices[i]->coords[3];
        while ((w >> w_size) && (w_size < 32))
            w_size += 4;
    }
    for (int i = 0; i < clipped_count; i++)
    {
        Vertex* v = poly->vertices[i];
        int32_t w = v->coords[3], normal_w;

        //W is brought as close as possible to 16-bit using 4-bit shifts
        if (w_size < 16)
        {
            w <<= 16 - w_size;
            normal_w = w >> (16 - w_size);
        }
        else
        {
            w >>= w_size - 16;
            normal_w = w << (w_size - 16);
        }

        //Convert z values
        //finalZ = (((vertexZ * 0x4000) / vertexW) + 0x3FFF) * 0x200
        int32_t z;
        if (flush_mode & 0x2) //w-buffer
            z = v->coords[3];
        else if (w)
            z = ((((int64_t)v->coords[2] * 0x4000) / v->coords[3]) + 0x3FFF) * 0x200;
        else
            z = 0x7FFE00;

        if (z < 0)
            z = 0;
        if (z > 0xFFFFFF)
            z = 0xFFFFFF;

        poly->final_z[i] = z;
        poly->final_w[i] = w;
    }
    poly->attributes = current_poly_attr;
    poly->texparams = TEXIMAGE_PARAM;
    poly->palette_base = PLTT_BASE;
    if ((current_poly_attr.alpha > 0 && current_poly_attr.alpha < 0x1F) ||
         TEXIMAGE_PARAM.format == 1 || TEXIMAGE_PARAM.format == 6)
        poly->translucent = true;
    else
        poly->translucent = false;

    //Shadow masks have an ID of zero
    //Actual shadows have a non-zero ID
    poly->shadow_mask = (!current_poly_attr.id && current_poly_attr.polygon_mode == 3);
    poly->shadow_poly = !poly->shadow_mask && current_poly_attr.polygon_mode == 3;
    geo_poly_count++;
    if (POLYGON_TYPE >= 2)
        last_poly_strip = poly;
    else
        last_poly_strip = nullptr;
}

void GPU_3D::add_vertex()
{
    int64_t coords[4];
    coords[0] = (int64_t)(int16_t)current_vertex[0];
    coords[1] = (int64_t)(int16_t)current_vertex[1];
    coords[2] = (int64_t)(int16_t)current_vertex[2];
    coords[3] = 0x1000;

    update_clip_mtx();

    Vertex* vtx = &vertex_list[vertex_list_count];

    vtx->coords[0] = (coords[0]*clip_mtx.m[0][0] + coords[1]*clip_mtx.m[1][0] +
            coords[2]*clip_mtx.m[2][0] + coords[3]*clip_mtx.m[3][0]) >> 12;
    vtx->coords[1] = (coords[0]*clip_mtx.m[0][1] + coords[1]*clip_mtx.m[1][1] +
            coords[2]*clip_mtx.m[2][1] + coords[3]*clip_mtx.m[3][1]) >> 12;
    vtx->coords[2] = (coords[0]*clip_mtx.m[0][2] + coords[1]*clip_mtx.m[1][2] +
            coords[2]*clip_mtx.m[2][2] + coords[3]*clip_mtx.m[3][2]) >> 12;
    vtx->coords[3] = (coords[0]*clip_mtx.m[0][3] + coords[1]*clip_mtx.m[1][3] +
            coords[2]*clip_mtx.m[2][3] + coords[3]*clip_mtx.m[3][3]) >> 12;

    if (TEXIMAGE_PARAM.transformation_mode == 3)
    {
        vtx->texcoords[0] = ((coords[0] * texture_mtx.m[0][0] + coords[1] * texture_mtx.m[1][0]
                + coords[2] * texture_mtx.m[2][0]) >> 24) + raw_texcoords[0];
        vtx->texcoords[1] = ((coords[0] * texture_mtx.m[0][1] + coords[1] * texture_mtx.m[1][1]
                + coords[2] * texture_mtx.m[2][1]) >> 24) + raw_texcoords[1];
    }
    else
    {
        vtx->texcoords[0] = (int16_t)current_texcoords[0];
        vtx->texcoords[1] = (int16_t)current_texcoords[1];
    }

    vtx->colors[0] = ((current_color & 0x1F) << 12) + 0xFFF;
    vtx->colors[1] = (((current_color >> 5) & 0x1F) << 12) + 0xFFF;
    vtx->colors[2] = (((current_color >> 10) & 0x1F) << 12) + 0xFFF;
    vtx->clipped = false;

    vertex_list_count++;
    switch (POLYGON_TYPE)
    {
        case 0:
            if (vertex_list_count == 3)
            {
                add_polygon();
                consecutive_polygons++;
                vertex_list_count = 0;
            }
            break;
        case 1:
            if (vertex_list_count == 4)
            {
                add_polygon();
                consecutive_polygons++;
                vertex_list_count = 0;
            }
            break;
        case 2:
            if (consecutive_polygons & 0x1)
            {
                swap(vertex_list[0], vertex_list[1]);

                add_polygon();
                consecutive_polygons++;
                vertex_list_count = 2;
                vertex_list[1] = vertex_list[2];
            }
            else if (vertex_list_count == 3)
            {
                add_polygon();
                consecutive_polygons++;
                vertex_list_count = 2;
                vertex_list[0] = vertex_list[1];
                vertex_list[1] = vertex_list[2];
            }
            break;
        case 3:
            if (vertex_list_count == 4)
            {
                swap(vertex_list[2], vertex_list[3]);
                add_polygon();
                consecutive_polygons++;
                vertex_list_count = 2;

                vertex_list[0] = vertex_list[3];
                vertex_list[1] = vertex_list[2];
            }
            break;
        default:
            printf("\nUnrecognized POLYGON_TYPE %d", POLYGON_TYPE);
    }
}

void GPU_3D::request_FIFO_DMA()
{
    if (GXFIFO.size() < 128)
        e->GXFIFO_DMA_request();
}

bool y_sort(Polygon a, Polygon b)
{
    if (a.translucent != b.translucent)
        return a.translucent < b.translucent;
    if (a.bottom_y != b.bottom_y)
        return a.bottom_y < b.bottom_y;
    return a.top_y < b.top_y;
}

void GPU_3D::end_of_frame()
{
    if (swap_buffers)
    {
        //printf("\nSWAP_BUFFERS");

        swap(rend_vert, geo_vert);
        swap(rend_poly, geo_poly);
        rend_vert_count = geo_vert_count;
        rend_poly_count = geo_poly_count;
        geo_vert_count = 0;
        geo_poly_count = 0;

        //Sort polygons by translucency
        int index = 0;
        int opaque_count = 0;
        Polygon temp[2048];
        for (int i = 0; i < rend_poly_count; i++)
        {
            if (!rend_poly[i].translucent)
            {
                temp[index] = rend_poly[i];
                index++;
                opaque_count++;
            }
        }

        for (int i = 0; i < rend_poly_count; i++)
        {
            if (rend_poly[i].translucent)
            {
                temp[index] = rend_poly[i];
                index++;
            }
        }

        memcpy(rend_poly, temp, rend_poly_count * sizeof(Polygon));

        //y-sorting
        if (flush_mode & 0x1)
            stable_sort(rend_poly, rend_poly + opaque_count, y_sort);
        else
            stable_sort(rend_poly, rend_poly + rend_poly_count, y_sort);
    }
    swap_buffers = false;
}

void GPU_3D::MTX_MULT(bool update_vector)
{
    MTX temp;
    int64_t temp_calc;
    MTX* target = nullptr;
    if (MTX_MODE != 3)
        clip_dirty = true;
    switch (MTX_MODE)
    {
        case 0:
            target = &projection_mtx;
            break;
        case 1:
            target = &modelview_mtx;
            break;
        case 2:
            target = &modelview_mtx;
            //Only multiply vector if command is not MTX_SCALE
            if (update_vector)
            {
                temp.set(vector_mtx);

                for (int i = 0; i < 4; i++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        temp_calc = 0;
                        for (int k = 0; k < 4; k++)
                            temp_calc += (int64_t)mult_params.m[i][k] * temp.m[k][j];
                        vector_mtx.m[i][j] = (int32_t)(temp_calc >> 12);
                    }
                }
            }
            break;
        case 3:
            target = &texture_mtx;
            break;
        default:
            printf("\nUnrecognized MTX_MODE %d in MTX_MULT", MTX_MODE);
    }

    temp.set(*target);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            temp_calc = 0;
            for (int k = 0; k < 4; k++)
                temp_calc += (int64_t)mult_params.m[i][k] * temp.m[k][j];
            target->m[i][j] = (int32_t)(temp_calc >> 12);
        }
    }

    //Reset the mult matrix for further use
    get_identity_mtx(mult_params);
    mult_params_index = 0;
}

void GPU_3D::update_clip_mtx()
{
    if (clip_dirty || true)
    {
        int64_t temp_calc;

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                temp_calc = 0;
                for (int k = 0; k < 4; k++)
                    temp_calc += (int64_t)modelview_mtx.m[i][k] * projection_mtx.m[k][j];
                clip_mtx.m[i][j] = temp_calc >> 12;
            }
        }
        clip_dirty = false;
    }
}

uint32_t* GPU_3D::get_framebuffer()
{
    return (uint32_t*)&framebuffer;
}

uint16_t GPU_3D::get_DISP3DCNT()
{
    uint16_t reg = 0;
    reg |= DISP3DCNT.texture_mapping;
    reg |= DISP3DCNT.highlight_shading << 1;
    reg |= DISP3DCNT.alpha_test << 2;
    reg |= DISP3DCNT.alpha_blending << 3;
    reg |= DISP3DCNT.anti_aliasing << 4;
    reg |= DISP3DCNT.edge_marking << 5;
    reg |= DISP3DCNT.fog_alpha_only << 6;
    reg |= DISP3DCNT.fog_enable << 7;
    reg |= DISP3DCNT.fog_depth_shift << 8;
    reg |= DISP3DCNT.color_buffer_underflow << 12;
    reg |= DISP3DCNT.RAM_overflow << 13;
    reg |= DISP3DCNT.rear_plane_mode << 14;
    return reg;
}

uint32_t GPU_3D::get_GXSTAT()
{
    //printf("\nGet GXSTAT");
    uint32_t reg = 0;
    reg |= GXSTAT.box_pos_vec_busy;
    reg |= (GXSTAT.boxtest_result) << 1;
    reg |= (modelview_sp & 0x1F) << 8;
    reg |= projection_sp << 13;
    reg |= GXSTAT.mtx_stack_busy << 14;
    reg |= GXSTAT.mtx_overflow << 15;
    reg |= GXFIFO.size() << 16;
    reg |= (GXFIFO.size() < 128) << 25;
    reg |= (GXFIFO.size() == 0) << 26;
    reg |= GXSTAT.geo_busy << 27;
    reg |= GXSTAT.GXFIFO_irq_stat << 30;
    return reg;
}

uint16_t GPU_3D::get_vert_count()
{
    return geo_vert_count;
}

uint16_t GPU_3D::get_poly_count()
{
    return geo_poly_count;
}

uint16_t GPU_3D::read_vec_test(uint32_t address)
{
    return vec_test_result[(address - 0x04000630) / 2];
}

uint32_t GPU_3D::read_clip_mtx(uint32_t address)
{
    update_clip_mtx();
    int x = ((address - 0x04000640) / 4) % 4;
    int y = ((address - 0x04000640) / 4) / 4;
    return clip_mtx.m[y][x];
}

uint32_t GPU_3D::read_vec_mtx(uint32_t address)
{
    uint32_t addr = address - 0x04000680;
    int x = addr % 3;
    int y = addr / 3;
    return vector_mtx.m[y][x];
}

void GPU_3D::set_DISP3DCNT(uint16_t halfword)
{
    DISP3DCNT.texture_mapping = halfword & 1;
    DISP3DCNT.highlight_shading = halfword & (1 << 1);
    DISP3DCNT.alpha_test = halfword & (1 << 2);
    DISP3DCNT.alpha_blending = halfword & (1 << 3);
    DISP3DCNT.anti_aliasing = halfword & (1 << 4);
    DISP3DCNT.edge_marking = halfword & (1 << 5);
    DISP3DCNT.fog_alpha_only = halfword & (1 << 6);
    DISP3DCNT.fog_enable = halfword & (1 << 7);
    DISP3DCNT.fog_depth_shift = (halfword >> 8) & 0xF;

    //TODO: Underflow/overflow: check me
    DISP3DCNT.color_buffer_underflow &= ~(halfword & (1 << 12));
    DISP3DCNT.RAM_overflow &= ~(halfword & (1 << 13));
    DISP3DCNT.rear_plane_mode = halfword & (1 << 14);
}

void GPU_3D::set_CLEAR_COLOR(uint32_t word)
{
    printf("\nSet CLEAR_COLOR: $%08X", word);
    CLEAR_COLOR = word;
}

void GPU_3D::set_CLEAR_DEPTH(uint32_t word)
{
    printf("\nSet CLEAR_DEPTH: $%08X", word);
    CLEAR_DEPTH = word & 0x7FFF;
}

void GPU_3D::set_EDGE_COLOR(uint32_t address, uint16_t halfword)
{
    int i = (address & 0xF) / 2;
    printf("\nSet EDGE_COLOR: $%04X ($%02X)", halfword, i);
    EDGE_COLOR[i] = halfword;
}

void GPU_3D::set_FOG_COLOR(uint32_t word)
{
    printf("\nSet FOG_COLOR: $%08X", word);
    FOG_COLOR = word;
}

void GPU_3D::set_FOG_OFFSET(uint16_t halfword)
{
    printf("\nSet FOG_OFFSET: $%04X", halfword);
    FOG_OFFSET = halfword;
}

void GPU_3D::set_FOG_TABLE(uint32_t address, uint8_t byte)
{
    int i = address & 0x1F;
    printf("\nSet FOG_TABLE: $%02X ($%02X)", byte, i);
    FOG_TABLE[i] = byte;
}

void GPU_3D::set_MTX_MODE(uint32_t word)
{
    printf("\nSet MTX_MODE: $%08X", word);
    MTX_MODE = word & 0x3;
}

void GPU_3D::MTX_PUSH()
{
    //printf("\nMTX_PUSH");
    switch (MTX_MODE)
    {
        case 0:
            if (!projection_sp)
            {
                projection_stack.set(projection_mtx);
                projection_sp++;
            }
            else
            {
                printf("\nMTX_PUSH overflow!");
                GXSTAT.mtx_overflow = true;
            }
            break;
        case 1:
        case 2:
            //printf("\nModelview SP: $%02X", modelview_sp);
            if (modelview_sp < 0x1F)
            {
                modelview_stack[modelview_sp].set(modelview_mtx);
                vector_stack[modelview_sp].set(vector_mtx);
            }
            else
            {
                printf("\nMTX_PUSH overflow!");
                GXSTAT.mtx_overflow = true;
            }
            modelview_sp = (modelview_sp + 1) & 0x1F;
            break;
        case 3:
            texture_stack.set(texture_mtx);
            break;
        default:
            printf("\nUnrecognized MTX_MODE %d for MTX_PUSH", MTX_MODE);
    }
}

void GPU_3D::MTX_POP(uint32_t word)
{
    printf("\nMTX_POP: $%08X", word);
    int8_t offset = ((int8_t)(word & 0x3F) << 2) >> 2;
    switch (MTX_MODE)
    {
        case 2:
            modelview_sp -= offset;
            modelview_mtx.set(modelview_stack[modelview_sp & 0x1F]);
            break;
        default:
            printf("\nUnrecognized MTX_MODE %d for MTX_POP", MTX_MODE);
    }
}

void GPU_3D::MTX_IDENTITY()
{
    //printf("\nMTX_IDENTITY");
    switch (MTX_MODE)
    {
        case 0:
            get_identity_mtx(projection_mtx);
            break;
        case 1:
            get_identity_mtx(modelview_mtx);
            break;
        case 2:
            get_identity_mtx(modelview_mtx);
            get_identity_mtx(vector_mtx);
            break;
        case 3:
            get_identity_mtx(texture_mtx);
            break;
        default:
            printf("\nUnrecognized MTX_MODE %d in MTX_IDENTITY", MTX_MODE);
    }
}

void GPU_3D::MTX_MULT_4x4(uint32_t word)
{
    printf("\nMTX_MULT_4x4: $%08X", word);

    add_mult_param(word);

    if (mult_params_index >= 16)
        MTX_MULT();
}

void GPU_3D::MTX_MULT_4x3(uint32_t word)
{
    printf("\nMTX_MULT_4x3: $%08X", word);

    add_mult_param(word);

    if ((mult_params_index & 0x3) == 0x3)
        mult_params_index++;

    if (mult_params_index >= 16)
        MTX_MULT();
}

void GPU_3D::MTX_MULT_3x3(uint32_t word)
{
    printf("\nMTX_MULT_3x3: $%08X", word);

    add_mult_param(word);

    if ((mult_params_index & 0x3) == 0x3)
        mult_params_index++;

    if (mult_params_index >= 11)
        MTX_MULT();
}

void GPU_3D::MTX_TRANS(uint32_t word)
{
    printf("\nMTX_TRANS: $%08X", word);

    if (mult_params_index == 0)
        mult_params_index = 12;

    add_mult_param(word);

    if (mult_params_index >= 15)
        MTX_MULT();
}

void GPU_3D::COLOR(uint32_t word)
{
    printf("\nCOLOR: $%08X", word);

    //geo_vert[geo_vert_count].color = word;
}

void GPU_3D::NORMAL()
{
    //Let's calculate some lighting!
    normal_vector[0] = (int16_t)((cmd_params[0] & 0x3FF) << 6) >> 6;
    normal_vector[1] = (int16_t)(((cmd_params[0] >> 10) & 0x3FF) << 6) >> 6;
    normal_vector[2] = (int16_t)(((cmd_params[0] >> 20) & 0x3FF) << 6) >> 6;
    if (TEXIMAGE_PARAM.transformation_mode == 2)
    {
        current_texcoords[0] = raw_texcoords[0] +
                (((int64_t)normal_vector[0] * texture_mtx.m[0][0] + (int64_t)normal_vector[1] * texture_mtx.m[1][0]
                + (int64_t)normal_vector[2] * texture_mtx.m[2][0]) >> 21);
        current_texcoords[1] = raw_texcoords[1] +
                (((int64_t)normal_vector[0] * texture_mtx.m[0][1] + (int64_t)normal_vector[1] * texture_mtx.m[1][1]
                + (int64_t)normal_vector[2] * texture_mtx.m[2][1]) >> 21);
    }

    int32_t normal[3];

    normal[0] = (normal_vector[0] * vector_mtx.m[0][0] + normal_vector[1] * vector_mtx.m[1][0] +
            normal_vector[2] * vector_mtx.m[2][0]) >> 12;
    normal[1] = (normal_vector[0] * vector_mtx.m[0][1] + normal_vector[1] * vector_mtx.m[1][1] +
            normal_vector[2] * vector_mtx.m[2][1]) >> 12;
    normal[2] = (normal_vector[0] * vector_mtx.m[0][2] + normal_vector[1] * vector_mtx.m[1][2] +
            normal_vector[2] * vector_mtx.m[2][2]) >> 12;

    uint32_t r, g, b, lr, lg, lb;
    r = emission_color & 0x1F;
    g = (emission_color >> 5) & 0x1F;
    b = (emission_color >> 10) & 0x1F;

    for (int light = 0; light < 4; light++)
    {
        if (!(POLYGON_ATTR.light_enable & (1 << light)))
            continue;

        lr = light_color[light] & 0x1F;
        lg = (light_color[light] >> 5) & 0x1F;
        lb = (light_color[light] >> 10) & 0x1F;

        int32_t diffuse_level = (-(light_direction[light][0] * normal[0] +
                light_direction[light][1] * normal[1] +
                light_direction[light][2] * normal[2])) >> 10;

        //Overflow handling taken from melonDS (same goes for specular)
        if (diffuse_level < 0)
            diffuse_level = 0;
        if (diffuse_level > 0xFF)
            diffuse_level = 0xFF;

        int32_t shine_level = -((((light_direction[light][0] >> 1) * normal[0] +
                (light_direction[light][1] >> 1) * normal[1] +
                ((light_direction[light][2] - 0x200) >> 1) * normal[2])) >> 10);

        if (shine_level < 0)
            shine_level = 0;
        else if (shine_level > 0xFF)
            shine_level = (0x100 - shine_level) & 0xFF;

        //2*shine_level^2 - 1.0
        shine_level = ((shine_level * shine_level) >> 7) - 0x100;

        if (shine_level < 0)
            shine_level = 0;

        if (using_shine_table)
        {
            shine_level >>= 1;
            shine_level = shine_table[shine_level];
        }

        r += (lr * (specular_color & 0x1F) * shine_level) >> 13;
        g += (lg * ((specular_color >> 5) & 0x1F) * shine_level) >> 13;
        b += (lb * ((specular_color >> 10) & 0x1F) * shine_level) >> 13;

        r += (lr * (diffuse_color & 0x1F) * diffuse_level) >> 13;
        g += (lg * ((diffuse_color >> 5) & 0x1F) * diffuse_level) >> 13;
        b += (lb * ((diffuse_color >> 10) & 0x1F) * diffuse_level) >> 13;

        r += (lr * (ambient_color & 0x1F)) >> 5;
        g += (lg * ((ambient_color >> 5) & 0x1F)) >> 5;
        b += (lb * ((ambient_color >> 10) & 0x1F)) >> 5;

        if (r > 0x1F)
            r = 0x1F;
        if (g > 0x1F)
            g = 0x1F;
        if (b > 0x1F)
            b = 0x1F;
    }

    current_color = r + (g << 5) + (b << 10);
}

void GPU_3D::set_POLYGON_ATTR(uint32_t word)
{
    //printf("\nSet POLYGON_ATTR: $%08X", word);
    POLYGON_ATTR.light_enable = word & 0xF;
    POLYGON_ATTR.polygon_mode = (word >> 4) & 0x3;
    POLYGON_ATTR.render_back = word & (1 << 6);
    POLYGON_ATTR.render_front = word & (1 << 7);
    POLYGON_ATTR.set_new_trans_depth = word & (1 << 11);
    POLYGON_ATTR.render_far_intersect = word & (1 << 12);
    POLYGON_ATTR.render_1dot = word & (1 << 13);
    POLYGON_ATTR.depth_test_equal = word & (1 << 14);
    POLYGON_ATTR.fog_enable = word & (1 << 15);
    POLYGON_ATTR.alpha = (word >> 16) & 0x1F;
    POLYGON_ATTR.id = (word >> 24) & 0x3F;
}

void GPU_3D::set_TEXIMAGE_PARAM(uint32_t word)
{
    //printf("\nSet TEXIMAGE_PARAM: $%08X", word);
    TEXIMAGE_PARAM.VRAM_offset = word & 0xFFFF;
    TEXIMAGE_PARAM.repeat_s = word & (1 << 16);
    TEXIMAGE_PARAM.repeat_t = word & (1 << 17);
    TEXIMAGE_PARAM.flip_s = word & (1 << 18);
    TEXIMAGE_PARAM.flip_t = word & (1 << 19);
    TEXIMAGE_PARAM.s_size = (word >> 20) & 0x7;
    TEXIMAGE_PARAM.t_size = (word >> 23) & 0x7;
    TEXIMAGE_PARAM.format = (word >> 26) & 0x7;
    TEXIMAGE_PARAM.color0_transparent = word & (1 << 29);
    TEXIMAGE_PARAM.transformation_mode = (word >> 30);
}

void GPU_3D::set_TOON_TABLE(uint32_t address, uint16_t color)
{
    TOON_TABLE[address] = color;
}

void GPU_3D::BEGIN_VTXS(uint32_t word)
{
    printf("\nBEGIN_VTXS: $%08X", word);
    POLYGON_TYPE = word & 0x3;
    /*geo_poly_RAM[geo_poly_index] = word;
    geo_poly_RAM[geo_poly_index + 1] = geo_vert_index;
    geo_poly_index += 2;*/
}

void GPU_3D::SWAP_BUFFERS(uint32_t word)
{
    swap_buffers = true;
    flush_mode = word & 0x3;
}

void GPU_3D::VIEWPORT(uint32_t word)
{
    //printf("\nVIEWPORT: $%08X", word);

    //viewport y-coords are upside down

    viewport.x1 = word & 0xFF;
    viewport.y1 = (191 - ((word >> 8) & 0xFF)) & 0xFF;
    viewport.x2 = (word >> 16) & 0xFF;
    viewport.y2 = (191 - ((word >> 24) & 0xFF)) & 0xFF;
}

void GPU_3D::BOX_TEST()
{
    GXSTAT.boxtest_result = false;

    Vertex cube[8], face[10];
    int16_t coords0[3], coords1[3];

    coords0[0] = (int16_t)(cmd_params[0] & 0xFFFF);
    coords0[1] = (int16_t)(cmd_params[0] >> 16);
    coords0[2] = (int16_t)(cmd_params[1] & 0xFFFF);
    coords1[0] = (int16_t)(cmd_params[1] >> 16);
    coords1[1] = (int16_t)(cmd_params[2] & 0xFFFF);
    coords1[2] = (int16_t)(cmd_params[2] >> 16);

    for (int i = 0; i < 3; i++)
        coords1[i] += coords0[i];

    cube[0].coords[0] = coords0[0]; cube[0].coords[1] = coords0[1]; cube[0].coords[2] = coords0[2];
    cube[1].coords[0] = coords1[0]; cube[1].coords[1] = coords0[1]; cube[1].coords[2] = coords0[2];
    cube[2].coords[0] = coords1[0]; cube[2].coords[1] = coords1[1]; cube[2].coords[2] = coords0[2];
    cube[3].coords[0] = coords0[0]; cube[3].coords[1] = coords1[1]; cube[3].coords[2] = coords0[2];
    cube[4].coords[0] = coords0[0]; cube[4].coords[1] = coords1[1]; cube[4].coords[2] = coords1[2];
    cube[5].coords[0] = coords0[0]; cube[5].coords[1] = coords0[1]; cube[5].coords[2] = coords1[2];
    cube[6].coords[0] = coords1[0]; cube[6].coords[1] = coords0[1]; cube[6].coords[2] = coords1[2];
    cube[7].coords[0] = coords1[0]; cube[7].coords[1] = coords1[1]; cube[7].coords[2] = coords1[2];

    update_clip_mtx();

    for (int i = 0; i < 8; i++)
    {
        int64_t x = (int32_t)cube[i].coords[0];
        int64_t y = (int32_t)cube[i].coords[1];
        int64_t z = (int32_t)cube[i].coords[2];
        int64_t w = 0x1000;

        cube[i].coords[0] = (x * clip_mtx.m[0][0] + y * clip_mtx.m[1][0] +
                z * clip_mtx.m[2][0] + w * clip_mtx.m[3][0]) >> 12;
        cube[i].coords[1] = (x * clip_mtx.m[0][1] + y * clip_mtx.m[1][1] +
                z * clip_mtx.m[2][1] + w * clip_mtx.m[3][1]) >> 12;
        cube[i].coords[2] = (x * clip_mtx.m[0][2] + y * clip_mtx.m[1][2] +
                z * clip_mtx.m[2][2] + w * clip_mtx.m[3][2]) >> 12;
        cube[i].coords[3] = (x * clip_mtx.m[0][3] + y * clip_mtx.m[1][3] +
                z * clip_mtx.m[2][3] + w * clip_mtx.m[3][3]) >> 12;
    }

    int vertices;

    //Front face
    face[0] = cube[0]; face[1] = cube[1]; face[2] = cube[2]; face[3] = cube[3];
    vertices = clip(face, 4, 0);
    if (vertices > 0)
    {
        GXSTAT.boxtest_result = true;
        return;
    }

    //Back face
    face[0] = cube[4]; face[1] = cube[5]; face[2] = cube[6]; face[3] = cube[7];
    vertices = clip(face, 4, 0);
    if (vertices > 0)
    {
        GXSTAT.boxtest_result = true;
        return;
    }

    //Left face
    face[0] = cube[0]; face[1] = cube[3]; face[2] = cube[4]; face[3] = cube[5];
    vertices = clip(face, 4, 0);
    if (vertices > 0)
    {
        GXSTAT.boxtest_result = true;
        return;
    }

    //Right face
    face[0] = cube[1]; face[1] = cube[2]; face[2] = cube[7]; face[3] = cube[6];
    vertices = clip(face, 4, 0);
    if (vertices > 0)
    {
        GXSTAT.boxtest_result = true;
        return;
    }

    //Bottom face
    face[0] = cube[0]; face[1] = cube[1]; face[2] = cube[6]; face[3] = cube[5];
    vertices = clip(face, 4, 0);
    if (vertices > 0)
    {
        GXSTAT.boxtest_result = true;
        return;
    }

    //Top face
    face[0] = cube[2]; face[1] = cube[3]; face[2] = cube[4]; face[3] = cube[7];
    vertices = clip(face, 4, 0);
    if (vertices > 0)
    {
        GXSTAT.boxtest_result = true;
        return;
    }
}

void GPU_3D::VEC_TEST()
{
    //printf("\nVEC_TEST");
    int16_t bark[3];
    bark[0] = (int16_t)((cmd_params[0] & 0x3FF) << 6) >> 6;
    bark[1] = (int16_t)(((cmd_params[0] >> 9) & 0x3FF) << 6) >> 6;
    bark[2] = (int16_t)(((cmd_params[0] >> 18) & 0x3FF) << 6) >> 6;

    vec_test_result[0] = (bark[0] * vector_mtx.m[0][0] + bark[1] * vector_mtx.m[1][0] + bark[2] * vector_mtx.m[2][0]) >> 9;
    vec_test_result[1] = (bark[0] * vector_mtx.m[0][1] + bark[1] * vector_mtx.m[1][1] + bark[2] * vector_mtx.m[2][1]) >> 9;
    vec_test_result[2] = (bark[0] * vector_mtx.m[0][2] + bark[1] * vector_mtx.m[1][2] + bark[2] * vector_mtx.m[2][2]) >> 9;

    vec_test_result[0] |= (vec_test_result[0] & 0x1000) * 0xF;
    vec_test_result[1] |= (vec_test_result[1] & 0x1000) * 0xF;
    vec_test_result[2] |= (vec_test_result[2] & 0x1000) * 0xF;
}

void GPU_3D::set_GXSTAT(uint32_t word)
{
    GXSTAT.mtx_overflow &= ~(word & (1 << 15));
    GXSTAT.GXFIFO_irq_stat = (word >> 30) & 0x3;
    check_FIFO_IRQ();
}
