/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef GPU3D_HPP
#define GPU3D_HPP
#include <cstdint>
#include <queue>
#include "memconsts.h"

struct DISP3DCNT_REG
{
    bool texture_mapping;
    bool highlight_shading;
    bool alpha_test;
    bool alpha_blending;
    bool anti_aliasing;
    bool edge_marking;
    bool fog_alpha_only;
    bool fog_enable;
    int fog_depth_shift;
    bool color_buffer_underflow;
    bool RAM_overflow;
    bool rear_plane_mode;
};

struct TEXIMAGE_PARAM_REG
{
    int VRAM_offset;
    bool repeat_s;
    bool repeat_t;
    bool flip_s;
    bool flip_t;
    int s_size;
    int t_size;
    int format;
    bool color0_transparent;
    int transformation_mode;
};

struct POLYGON_ATTR_REG
{
    int light_enable;
    int polygon_mode;
    bool render_back;
    bool render_front;
    bool set_new_trans_depth;
    bool render_1dot;
    bool render_far_intersect;
    bool depth_test_equal;
    bool fog_enable;
    int alpha;
    int id;
};

struct VIEWPORT_REG
{
    uint8_t x1, y1, x2, y2;
};

struct GXSTAT_REG
{
    bool box_pos_vec_busy;
    bool boxtest_result;
    bool mtx_stack_busy;
    bool mtx_overflow;
    bool geo_busy;
    int GXFIFO_irq_stat;
};

struct MTX
{
    int32_t m[4][4];

    void set(const MTX& mtx);
};

struct Vertex
{
    int32_t coords[4];
    int32_t colors[3];
    int32_t texcoords[2];
    int32_t final_pos[2];
    int32_t final_colors[3];
    bool clipped;
};

struct Polygon
{
    Vertex* vertices[10];
    uint8_t vert_count;

    int32_t final_z[10];
    int32_t final_w[10];

    uint16_t top_y, bottom_y;

    POLYGON_ATTR_REG attributes;
    TEXIMAGE_PARAM_REG texparams;
    uint32_t palette_base;

    bool translucent;
    bool shadow_mask;
    bool shadow_poly;
};

struct GX_Command
{
    uint8_t command;
    uint32_t param;
};

class Emulator;

class GPU;

struct RenderAttr
{
    int trans_id;
    bool translucent;
    int opaque_id;
    bool fog;
    bool edge;
};

class GPU_3D
{
    private:
        Emulator* e;
        GPU* gpu;
        uint32_t framebuffer[PIXELS_PER_LINE];
        int cycles;
        DISP3DCNT_REG DISP3DCNT;
        POLYGON_ATTR_REG POLYGON_ATTR;
        TEXIMAGE_PARAM_REG TEXIMAGE_PARAM;
        uint16_t TOON_TABLE[32];
        uint32_t PLTT_BASE;
        VIEWPORT_REG viewport;
        GXSTAT_REG GXSTAT;
        uint32_t POLYGON_TYPE;
        uint32_t CLEAR_DEPTH, CLEAR_COLOR;
        int flush_mode;

        std::queue<GX_Command> GXFIFO;
        std::queue<GX_Command> GXPIPE;

        uint32_t cmd_params[32];
        uint8_t param_count;
        uint8_t cmd_param_count;
        uint8_t cmd_count;
        uint8_t total_params;
        uint32_t current_cmd;
        POLYGON_ATTR_REG current_poly_attr;

        uint32_t current_color;
        int16_t current_vertex[3];
        int16_t current_texcoords[2];
        int16_t raw_texcoords[2];

        uint32_t z_buffer[SCANLINES][PIXELS_PER_LINE];

        //Attribute buffer flags:
        //0-4: trans poly id
        //5: translucent
        //6-11: opaque poly id
        //12: fog
        //13: edge
        RenderAttr attr_buffer[PIXELS_PER_LINE];

        uint16_t EDGE_COLOR[8];

        uint32_t FOG_COLOR;
        uint16_t FOG_OFFSET;
        uint8_t FOG_TABLE[32];

        //Stencil buffer has two separate places for odd/even scanlines?
        bool stencil_buffer[PIXELS_PER_LINE * 2];
        bool previous_shadow_mask;

        bool swap_buffers;

        static const uint8_t cmd_param_amounts[256];
        static const uint16_t cmd_cycle_amounts[256];

        //Dynamically allocated vertex/polygon RAM banks
        Vertex *vert_bank1, *vert_bank2;
        Polygon *poly_bank1, *poly_bank2;

        //Pointers to the RAM banks
        Vertex* geo_vert, *rend_vert;
        Polygon* geo_poly, *rend_poly;

        Polygon* last_poly_strip;

        Vertex vertex_list[10];
        int vertex_list_count;

        int geo_vert_count, rend_vert_count;
        int geo_poly_count, rend_poly_count;

        int consecutive_polygons;

        int VTX_16_index;

        uint8_t MTX_MODE;

        MTX projection_mtx, vector_mtx, modelview_mtx, texture_mtx;
        MTX projection_stack, texture_stack;
        MTX modelview_stack[0x20];
        MTX vector_stack[0x20];
        MTX clip_mtx;
        bool clip_dirty;
        uint8_t projection_sp, modelview_sp;

        uint16_t emission_color, ambient_color, diffuse_color, specular_color;
        uint16_t light_color[4];
        int16_t light_direction[4][3];
        int16_t normal_vector[3];
        uint8_t shine_table[128];
        bool using_shine_table;

        int16_t vec_test_result[3];

        const static MTX IDENTITY;

        MTX mult_params;
        int mult_params_index;

        int64_t interpolate(uint64_t pixel, uint64_t pixel_range, int64_t u1, int64_t u2, int32_t w1, int32_t w2);

        void get_identity_mtx(MTX& mtx);

        GX_Command read_command();
        void write_command(GX_Command& cmd);
        void exec_command();

        void add_mult_param(uint32_t word);
        void MTX_MULT(bool update_vector = true);
        void update_clip_mtx();

        int clip(Vertex* v_list, int v_len, int clip_start, bool add_attributes = false);
        int clip_plane(int plane, Vertex* v_list, int v_len, int clip_start, bool add_attributes);
        void clip_vertex(int plane, Vertex& v_list, Vertex& v_out, Vertex* v_in, int side, bool add_attributes);
        void add_vertex();
        void add_polygon();
        void render_shadow_mask(Polygon* poly);
        void request_FIFO_DMA();
    public:
        GPU_3D(Emulator* e, GPU* gpu);
        ~GPU_3D();
        void power_on();
        void render_scanline();
        void run(uint64_t cycles_to_run);
        void end_of_frame();
        void check_FIFO_DMA();
        void check_FIFO_IRQ();

        void write_GXFIFO(uint32_t word);
        void write_FIFO_direct(uint32_t address, uint32_t word);

        uint32_t* get_framebuffer();
        uint16_t get_DISP3DCNT();
        uint32_t get_GXSTAT();
        uint16_t get_vert_count();
        uint16_t get_poly_count();
        uint32_t read_clip_mtx(uint32_t address);
        uint32_t read_vec_mtx(uint32_t address);
        uint16_t read_vec_test(uint32_t address);

        void set_DISP3DCNT(uint16_t halfword);

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
        void NORMAL();
        void set_POLYGON_ATTR(uint32_t word);
        void set_TEXIMAGE_PARAM(uint32_t word);
        void set_TOON_TABLE(uint32_t address, uint16_t color);
        void BEGIN_VTXS(uint32_t word);
        void SWAP_BUFFERS(uint32_t word);
        void VIEWPORT(uint32_t word);
        void BOX_TEST();
        void VEC_TEST();
        void set_GXSTAT(uint32_t word);
};

#endif // GPU3D_HPP
