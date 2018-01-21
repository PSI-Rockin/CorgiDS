/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef cartridge_hpp
#define cartridge_hpp
#include <memory>
#include <string>

enum CART_COMMAND
{
    EMPTY,
    DUMMY,
    GET_HEADER,
    GET_CHIP_ID,
    ENABLE_KEY1,
    ENABLE_KEY2,
    GET_SECURE_AREA_BLOCK,
    READ_ROM
};

enum class AUXSPI_COMMAND
{
    EMPTY,
    WRITE_MEM,
    READ_MEM,
    READ_STATUS_REG,
    PAGE_WRITE,
    WRITE_HI,
    READ_HI
};

struct REG_ROMCTRL
{
    int key1_gap;
    bool key2_data_enabled;
    bool key2_apply_seed;
    int key2_gap;
    bool key2_cmd_enabled;
    bool word_ready;
    int block_size;
    bool slow_transfer;
    bool key1_gap_clocks;
    bool block_busy;
};

struct REG_AUXSPICNT
{
    int bandwidth;
    bool hold_chipselect;
    bool is_busy;
    bool serial_transfer;
    bool IRQ_after_transfer;
    bool enabled;
};

class Emulator;

class NDS_Cart
{
    private:
        Emulator* e;
    
        uint8_t key1_buffer[0x1048];
        int cmd_encrypt_mode;
    
        std::unique_ptr<uint8_t[]> ROM;
        std::unique_ptr<uint8_t[]> save_database;
        std::string ROM_name;
        uint8_t SPI_save[1024 * 1024 * 8];
        int save_size;
        int save_type;
        bool dirty_save;
        long long database_size;
        long long ROM_size;
        uint8_t command_buffer[8];
        uint32_t data_output;
        int ROM_data_index;
        CART_COMMAND command_id;
    
        int secure_area_index;
    
        int cycles_left;
        int bytes_left;
    
        REG_ROMCTRL ROMCTRL;
        REG_AUXSPICNT AUXSPICNT;
        AUXSPI_COMMAND spi_cmd;
        uint8_t spi_data;
        int spi_params;
        uint32_t spi_addr;
        bool spi_write_enabled;

        uint64_t encrypt_seed0, encrypt_seed1;
    
        uint32_t keycode[3];
        void key1_encrypt(uint32_t* data);
        void key1_decrypt(uint32_t* data);
        void apply_keycode(uint32_t modulo);
        void init_keycode(uint32_t idcode, int level, uint32_t modulo);
    public:
        NDS_Cart(Emulator* e);
        void power_on();
        bool game_loaded();
        int load_database(std::string file_name);
        int load_ROM(std::string file_name);
        void save_check();
    
        uint8_t read_command(int index);
        void receive_command(uint8_t command, int index);
        void run(int cycles);
        void debug_encrypt();
    
        uint8_t direct_read(uint32_t address);
        uint16_t direct_read_halfword(uint32_t address);
        uint32_t direct_read_word(uint32_t address);
    
        uint32_t get_ROMCTRL();
        uint32_t get_output();
        uint16_t get_AUXSPICNT();
        uint8_t read_AUXSPIDATA();
    
        void set_hi_AUXSPICNT(uint8_t value);
        void set_AUXSPICNT(uint16_t value);
        void set_AUXSPIDATA(uint8_t value);
        void set_ROMCTRL(uint32_t value);
        void set_lo_key2_seed0(uint32_t word);
        void set_lo_key2_seed1(uint32_t word);
        void set_hi_key2_seed0(uint32_t word);
        void set_hi_key2_seed1(uint32_t word);
};

#endif /* cartridge_hpp */
