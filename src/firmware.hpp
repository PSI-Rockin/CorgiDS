/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef firmware_hpp
#define firmware_hpp
#include <cstdint>
#include <string>
#include <vector>

enum FIRM_COMMAND
{
    NONE,
    READ_STATUS_REG,
    READ_STREAM
};

class Emulator;

class Firmware
{
    public:
        constexpr static int SIZE = 1024 * 256; //262 KB
    private:
        Emulator* e;
        uint8_t firmware[SIZE];
        uint8_t status_reg;
        int user_data;
    
        FIRM_COMMAND command_id;
        uint32_t address;
        int total_args;

        uint16_t create_CRC(uint8_t* data, int length, uint32_t start);
        bool verify_CRC(uint32_t start, uint32_t offset, uint32_t length, uint32_t CRC_offset);
    public:
        Firmware(Emulator* e);
        int load_firmware(std::string file_name);
        void direct_boot();
    
        uint8_t transfer_data(uint8_t input);
        void deselect();
};

#endif /* firmware_hpp */
