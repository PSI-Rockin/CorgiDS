/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstring>
#include <fstream>
#include <string>
#include "cartridge.hpp"
#include "config.hpp"
#include "emulator.hpp"

using namespace std;

ifstream::pos_type get_file_size(string file_name)
{
    ifstream in(file_name, ifstream::ate | ifstream::binary);
    ifstream::pos_type size = in.tellg();
    in.close();
    return size;
}

uint32_t byteswap_word(uint32_t word)
{
    uint32_t result = 0;
    result |= word >> 24;
    result |= (word & 0xFF0000) >> 8;
    result |= (word & 0xFF00) << 8;
    result |= word << 24;
    return result;
}

NDS_Cart::NDS_Cart(Emulator* e) : e(e)
{
    power_on();
}

void NDS_Cart::key1_encrypt(uint32_t *data)
{
    uint32_t Y = data[0];
    uint32_t X = data[1];
    uint32_t Z;
    uint32_t* buffer = (uint32_t*)key1_buffer;
    for (int i = 0; i <= 0xF; i++)
    {
        Z = buffer[i] ^ X;
        X = buffer[0x012 + ((Z >> 24) & 0xFF)];
        X += buffer[0x112 + ((Z >> 16) & 0xFF)];
        X ^= buffer[0x212 + ((Z >> 8) & 0xFF)];
        X += buffer[0x312 + (Z & 0xFF)];
        X ^= Y;
        Y = Z;
    }
    data[0] = X ^ buffer[0x10];
    data[1] = Y ^ buffer[0x11];
}

void NDS_Cart::key1_decrypt(uint32_t *data)
{
    uint32_t Y = data[0];
    uint32_t X = data[1];
    uint32_t Z;
    uint32_t* buffer = (uint32_t*)key1_buffer;
    for (int i = 0x11; i >= 0x2; i--)
    {
        Z = buffer[i] ^ X;
        X = buffer[0x012 + ((Z >> 24) & 0xFF)];
        X += buffer[0x112 + ((Z >> 16) & 0xFF)];
        X ^= buffer[0x212 + ((Z >> 8) & 0xFF)];
        X += buffer[0x312 + (Z & 0xFF)];
        X ^= Y;
        Y = Z;
    }
    data[0] = X ^ buffer[1];
    data[1] = Y ^ buffer[0];
}

void NDS_Cart::apply_keycode(uint32_t modulo)
{
    key1_encrypt(&keycode[1]);
    key1_encrypt(&keycode[0]);
    uint32_t* buffer = (uint32_t*)&key1_buffer;
    for (int i = 0; i <= 0x11; i++)
        buffer[i] ^= byteswap_word(keycode[i % modulo]);
    
    uint32_t scratch[2] = {0, 0};
    for (int i = 0; i <= 0x410; i += 2)
    {
        key1_encrypt(scratch);
        buffer[i] = scratch[1];
        buffer[i + 1] = scratch[0];
    }
}

void NDS_Cart::init_keycode(uint32_t idcode, int level, uint32_t modulo)
{
    e->cart_copy_keybuffer(key1_buffer);
    keycode[0] = idcode;
    keycode[1] = idcode >> 1;
    keycode[2] = idcode << 1;
    if (level >= 1)
        apply_keycode(modulo);
    if (level >= 2)
        apply_keycode(modulo);
    keycode[1] <<= 1;
    keycode[2] >>= 1;
    if (level >= 3)
        apply_keycode(modulo);
}

void NDS_Cart::power_on()
{
    save_size = 1024 * 1024;
    save_type = 2;
    cycles_left = 8;
    bytes_left = 0;
    ROMCTRL.word_ready = true;
    ROMCTRL.block_busy = false;
    cmd_encrypt_mode = 0;
    AUXSPICNT.hold_chipselect = false;
    AUXSPICNT.is_busy = false;
    AUXSPICNT.serial_transfer = false;
    AUXSPICNT.IRQ_after_transfer = false;
    AUXSPICNT.enabled = false;

    memset(SPI_save, 0, 1024 * 64);
    dirty_save = false;

    if (ROM)
        ROM = nullptr;

    ROM_name = "";

    spi_cmd = AUXSPI_COMMAND::EMPTY;
    spi_data = 0;
    spi_params = 0;
}

bool NDS_Cart::game_loaded()
{
    return ROM != nullptr;
}

int NDS_Cart::load_database(string file_name)
{
    save_database = nullptr;
    database_size = 0;
    ifstream file(file_name, ios::binary | ios::in);
    if (!file.is_open())
    {
        printf("Failed to load save database.\n");
        return 1;
    }

    database_size = get_file_size(file_name);
    if (database_size % 19)
    {
        printf("Save database corrupted or in wrong format.\n");
        return 1;
    }

    printf("Save database successfully loaded.\n");

    save_database = unique_ptr<uint8_t[]>(new uint8_t[database_size]);
    file.read((char*)save_database.get(), database_size);
    file.close();

    return 0;
}

int NDS_Cart::load_ROM(string file_name)
{
    power_on();
    
    ifstream file(file_name, ios::binary | ios::in);
    if (!file.is_open())
    {
        printf("Failed to load %s\n", file_name.c_str());
        return 1;
    }
    
    printf("%s successfully loaded\n", file_name.c_str());
    
    ROM_size = get_file_size(file_name);
    printf("Allocating %lld bytes for ROM...\n", ROM_size);
    
    ROM = unique_ptr<uint8_t[]>(new uint8_t[ROM_size]);
    
    printf("Allocated memory successfully.\n");
    printf("Loading ROM into memory...\n");
    
    file.read((char*)ROM.get(), ROM_size);
    file.close();

    //Remove the .nds extension
    ROM_name = file_name.substr(0, file_name.length() - 4);

    //Load save if possible
    ifstream save_file(ROM_name + ".sav", ios::in | ios::binary);
    if (save_file.is_open())
    {
        int file_size = get_file_size(ROM_name + ".sav");
        if (file_size)
        {
            static const int save_sizes[7] =
                {512, 1024 * 8, 1024 * 64, 1024 * 256, 1024 * 512, 1024 * 1024, 1024 * 1024 * 8};
            save_file.read((char*)SPI_save, file_size);
            //Round save size up to the closest option
            for (int i = 0; i < 6; i++)
            {
                if (file_size > save_sizes[i] && file_size <= save_sizes[i + 1])
                {
                    file_size = save_sizes[i + 1];
                    break;
                }
            }
            save_size = file_size;
            printf("Loaded save for %s successfully.\n", ROM_name.c_str());
            printf("Save size: %d\n", save_size);
        }
        save_file.close();
    }
    else
    {
        //Check database for possible entry
        if (save_database)
        {
            bool match;
            for (int index = 0; index < database_size / 19; index++)
            {
                //Check if NDS header and database entry match
                match = true;
                for (int c = 0; c < 16; c++)
                {
                    if (ROM[c] != save_database[c + (index * 19)])
                    {
                        match = false;
                        break;
                    }
                }
                if (match)
                {
                    printf("\nFound ROM entry in database.\n");
                    int size_index = save_database[18 + (index * 19)];
                    switch (size_index)
                    {
                        case 0x02:
                            save_size = 512;
                            break;
                        case 0x03:
                            save_size = 1024 * 8;
                            break;
                        case 0x04:
                            save_size = 1024 * 64;
                            break;
                        case 0x05:
                            save_size = 1024 * 256;
                            break;
                        case 0x06:
                            save_size = 1024 * 512;
                            break;
                        case 0x07:
                            save_size = 1024 * 1024;
                            break;
                        default:
                            printf("Unrecognized save format %d!\n", size_index);
                            break;
                    }
                    printf("Save size: %d\n", save_size);
                }
            }
        }
    }

    if (save_size == 512)
        save_type = 0;
    if (save_size >= 1024 * 8 && save_size <= 1024 * 64)
        save_type = 1;
    if (save_size > 1024 * 64)
        save_type = 2;

    //Only re-encrypt the ROM if this is not a direct boot
    if (Config::direct_boot_enabled)
        return 0;
    
    uint32_t gamecode = *(uint32_t*)&ROM[0xC];
    uint32_t arm_ROM_base = *(uint32_t*)&ROM[0x20];
    if (arm_ROM_base < 0x8000)
    {
        if (arm_ROM_base >= 0x4000)
        {
            if (*(uint32_t*)&ROM[arm_ROM_base] == 0xE7FFDEFF && *(uint32_t*)&ROM[arm_ROM_base + 0x10] != 0xE7FFDEFF)
            {
                //The first two KB of the secure area must be re-encrypted
                //And the first eight bytes ("encryObj") must be double encrypted
                printf("\nEncrypting secure area");
                
                strncpy((char*)&ROM[arm_ROM_base], "encryObj", 8);
                
                init_keycode(gamecode, 3, 2);
                for (unsigned int i = 0; i < 0x800; i += 8)
                    key1_encrypt((uint32_t*)&ROM[arm_ROM_base + i]);
                
                init_keycode(gamecode, 2, 2);
                key1_encrypt((uint32_t*)&ROM[arm_ROM_base]);
            }
        }
    }

    init_keycode(gamecode, 2, 2);
    return 0;
}

void NDS_Cart::save_check()
{
    if (dirty_save)
    {
        dirty_save = false;
        ofstream save_file(ROM_name + ".sav", ios::binary | ios::out);
        if (save_file.is_open())
        {
            save_file.write((char*)SPI_save, save_size);
            save_file.close();
        }
    }
}

void NDS_Cart::run(int cycles)
{
    if (ROMCTRL.block_busy && !ROMCTRL.word_ready)
    {
        cycles_left -= cycles;
        if (cycles_left > 0)
            return;

        cycles_left = 8;
        switch (command_id)
        {
            case CART_COMMAND::DUMMY:
                data_output = 0xFFFFFFFF;
                break;
            case CART_COMMAND::GET_HEADER:
                data_output = *(uint32_t*)&ROM[ROM_data_index];
                ROM_data_index += 4;
                if (ROM_data_index > 0xFFF)
                    ROM_data_index = 0x0;
                ROMCTRL.word_ready = true;
                break;
            case CART_COMMAND::GET_CHIP_ID:
                //The chip id doesn't really matter as long as it stays consistent
                //For those curious, this particular id corresponds to a Macronix 64 MB ROM
                data_output = 0x00003FC2;
                ROMCTRL.word_ready = true;
                break;
            case CART_COMMAND::ENABLE_KEY1:
                cmd_encrypt_mode = 1;
                break;
            case CART_COMMAND::GET_SECURE_AREA_BLOCK:
                data_output = *(uint32_t*)&ROM[secure_area_index];
                secure_area_index += 4;
                ROMCTRL.word_ready = true;
                break;
            case CART_COMMAND::READ_ROM:
                if (ROM_data_index < 0x8000)
                    data_output = *(uint32_t*)&ROM[0x8000 + (ROM_data_index & 0x1FF)];
                else
                    data_output = *(uint32_t*)&ROM[ROM_data_index];
                ROM_data_index += 4;
                ROMCTRL.word_ready = true;
                break;
            default:
                printf("\nCommand $%02X%02X%02X%02X%02X%02X%02X%02X to cartridge not recognized",
                       command_buffer[0], command_buffer[1], command_buffer[2], command_buffer[3],
                       command_buffer[4], command_buffer[5], command_buffer[6], command_buffer[7]);
                throw "[CART] Unknown command sent";
        }
        bytes_left -= 4;
        e->gamecart_DMA_request();
        if (bytes_left <= 0)
        {
            ROMCTRL.block_busy = false;
            if (AUXSPICNT.IRQ_after_transfer)
            {
                if (e->arm7_has_cart_rights())
                    e->request_interrupt7(INTERRUPT::CART_TRANSFER);
                else
                    e->request_interrupt9(INTERRUPT::CART_TRANSFER);
            }
        }
    }
}

uint8_t NDS_Cart::direct_read(uint32_t address)
{
    return ROM[address];
}

uint16_t NDS_Cart::direct_read_halfword(uint32_t address)
{
    return *(uint16_t*)&ROM[address];
}

uint32_t NDS_Cart::direct_read_word(uint32_t address)
{
    return *(uint32_t*)&ROM[address];
}

uint8_t NDS_Cart::read_command(int index)
{
    return command_buffer[index];
}

void NDS_Cart::receive_command(uint8_t command, int index)
{
    command_buffer[index] = command;
}

uint32_t NDS_Cart::get_ROMCTRL()
{
    uint32_t reg = 0;
    
    reg |= ROMCTRL.key1_gap;
    reg |= ROMCTRL.key2_data_enabled << 13;
    reg |= ROMCTRL.key2_apply_seed << 15;
    reg |= ROMCTRL.key2_gap << 16;
    reg |= ROMCTRL.key2_cmd_enabled << 22;
    reg |= ROMCTRL.word_ready << 23;
    reg |= ROMCTRL.block_size << 24;
    reg |= ROMCTRL.slow_transfer << 27;
    reg |= ROMCTRL.key1_gap_clocks << 28;
    reg |= 1 << 29; //Always one? Who knows
    reg |= ROMCTRL.block_busy << 31;
    
    return reg;
}

uint32_t NDS_Cart::get_output()
{
    if (ROMCTRL.word_ready)
    {
        ROMCTRL.word_ready = false;
        cycles_left = 8;
    }
    return data_output;
}

uint16_t NDS_Cart::get_AUXSPICNT()
{
    uint16_t reg = 0;
    reg |= AUXSPICNT.bandwidth;
    reg |= AUXSPICNT.hold_chipselect << 6;
    reg |= AUXSPICNT.is_busy << 7;
    reg |= AUXSPICNT.serial_transfer << 13;
    reg |= AUXSPICNT.IRQ_after_transfer << 14;
    reg |= AUXSPICNT.enabled << 15;
    return reg;
}

uint8_t NDS_Cart::read_AUXSPIDATA()
{
    if (AUXSPICNT.enabled)
        return spi_data;
    return 0;
}

void NDS_Cart::set_hi_AUXSPICNT(uint8_t value)
{
    AUXSPICNT.serial_transfer = value & (1 << 5);
    AUXSPICNT.IRQ_after_transfer = value & (1 << 6);
    AUXSPICNT.enabled = value & (1 << 7);
}

void NDS_Cart::set_AUXSPICNT(uint16_t value)
{
    AUXSPICNT.bandwidth = value & 0x3;
    AUXSPICNT.hold_chipselect = value & (1 << 6);
    set_hi_AUXSPICNT(value >> 8);
}

//TODO: make this function a lot cleaner
void NDS_Cart::set_AUXSPIDATA(uint8_t value)
{
    if (spi_cmd == AUXSPI_COMMAND::EMPTY)
    {
        spi_params = 0;
        switch (value)
        {
            case 0:
                printf("\nderp");
                break;
            case 2:
                printf("\nAUXSPI write");
                spi_cmd = AUXSPI_COMMAND::WRITE_MEM;
                break;
            case 3:
                printf("\nAUXSPI read");
                spi_cmd = AUXSPI_COMMAND::READ_MEM;
                break;
            case 4:
                printf("\nAUXSPI write disabled");
                spi_write_enabled = false;
                break;
            case 5:
                printf("\nAUXSPI read status reg");
                spi_cmd = AUXSPI_COMMAND::READ_STATUS_REG;
                spi_addr = 0;
                break;
            case 6:
                printf("\nAUXSPI write enabled");
                spi_write_enabled = true;
                break;
            case 10:
                printf("\nAUXSPI page write");
                if (save_type == 2)
                    spi_cmd = AUXSPI_COMMAND::PAGE_WRITE;
                else
                    spi_cmd = AUXSPI_COMMAND::WRITE_HI;
                break;
            case 11:
                if (save_type == 0)
                    spi_cmd = AUXSPI_COMMAND::READ_HI;
                break;
            default:
                printf("\nUnrecognized AUXSPI cmd %d", value);
        }
    }
    else
    {
        switch (spi_cmd)
        {
            case AUXSPI_COMMAND::READ_STATUS_REG:
                spi_data = spi_write_enabled << 1;
                if (save_type == 0)
                    spi_data |= 0xF0;
                break;
            case AUXSPI_COMMAND::WRITE_MEM:
                switch (save_type)
                {
                    case 0:
                        if (spi_params < 2)
                            spi_addr = value;
                        else if (spi_write_enabled)
                        {
                            dirty_save = true;
                            SPI_save[spi_addr & 0xFF] = value;
                            spi_addr++;
                        }
                        break;
                    case 1:
                        if (spi_params < 3)
                            spi_addr |= value << ((2 - spi_params) * 8);
                        else if (spi_write_enabled)
                        {
                            dirty_save = true;
                            SPI_save[spi_addr & (save_size - 1)] = value;
                            spi_addr++;
                        }
                        break;
                    case 2:
                        if (spi_params < 4)
                            spi_addr |= value << ((3 - spi_params) * 8);
                        else if (spi_write_enabled)
                        {
                            dirty_save = true;
                            SPI_save[spi_addr & (save_size - 1)] = 0;
                            spi_addr++;
                        }
                        break;
                }
                /*if (!flash_save && spi_params < 3)
                    spi_addr |= value << ((2 - spi_params) * 8);
                else if (flash_save && spi_params < 4)
                    spi_addr |= value << ((3 - spi_params) * 8);
                else
                {
                    if (spi_write_enabled)
                    {
                        dirty_save = true;
                        if (!flash_save)
                            SPI_save[spi_addr & (save_size - 1)] = value;
                        else
                            SPI_save[spi_addr & (save_size - 1)] = 0;

                        spi_addr++;
                    }
                }*/
                //printf("\nWRITE_MEM: %d", value);
                break;
            case AUXSPI_COMMAND::READ_MEM:
                switch (save_type)
                {
                    case 0:
                        if (spi_params < 2)
                            spi_addr |= value;
                        else
                        {
                            spi_data = SPI_save[spi_addr & 0xFF];
                            spi_addr++;
                        }
                        break;
                    case 1:
                        if (spi_params < 3)
                            spi_addr |= value << ((2 - spi_params) * 8);
                        else
                        {
                            spi_data = SPI_save[spi_addr & (save_size - 1)];
                            spi_addr++;
                        }
                        break;
                    case 2:
                        if (spi_params < 4)
                        {
                            spi_addr <<= 8;
                            spi_addr |= value;
                        }
                        else
                        {
                            spi_data = SPI_save[spi_addr & (save_size - 1)];
                            spi_addr++;
                        }
                        break;
                }
                //printf("\nREAD_MEM: %d", value);
                break;
            case AUXSPI_COMMAND::PAGE_WRITE:
                if (save_type == 2)
                {
                    if (spi_params < 4)
                    {
                        spi_addr <<= 8;
                        spi_addr |= value;
                    }
                    else
                    {
                        if (spi_write_enabled)
                        {
                            //printf("\nPage write to $%08X", spi_addr);
                            dirty_save = true;
                            SPI_save[spi_addr & (save_size - 1)] = value;
                            spi_addr++;
                        }
                    }
                }
                break;
            case AUXSPI_COMMAND::WRITE_HI:
                if (spi_params < 2)
                    spi_addr = 0x100 | value;
                else
                {
                    if (spi_write_enabled)
                    {
                        dirty_save = true;
                        SPI_save[spi_addr & 0x1FF] = value;
                        spi_addr++;
                        if (spi_addr == 0x200)
                            spi_addr = 0x100;
                    }
                }
                break;
            case AUXSPI_COMMAND::READ_HI:
                if (spi_params < 2)
                    spi_addr = 0x100 | value;
                else
                {
                    spi_data = SPI_save[spi_addr & 0x1FF];
                    spi_addr++;
                    if (spi_addr == 0x200)
                        spi_addr = 0x100;
                }
                break;
        }
    }
    spi_params++;
    if (!AUXSPICNT.hold_chipselect)
    {
        //printf("\nDeselected AUXSPI");
        spi_params = 0;
        spi_addr = 0;
        spi_cmd = AUXSPI_COMMAND::EMPTY;
    }
}

void NDS_Cart::set_ROMCTRL(uint32_t value)
{
    bool old_transfer_busy = ROMCTRL.block_busy;
    
    ROMCTRL.key1_gap = value & 0x1FFF;
    ROMCTRL.key2_data_enabled = (value >> 13) & 0x1;
    ROMCTRL.key2_apply_seed = (value >> 15) & 0x1;
    ROMCTRL.key2_gap = (value >> 16) & 0x3F;
    ROMCTRL.key2_cmd_enabled = (value >> 22) & 0x1;
    ROMCTRL.block_size = (value >> 24) & 0x7;
    ROMCTRL.slow_transfer = (value >> 27) & 0x1;
    ROMCTRL.key1_gap_clocks = (value >> 28) & 0x1;
    ROMCTRL.block_busy = (value >> 31) & 0x1;
    
    if (!old_transfer_busy && ROMCTRL.block_busy && AUXSPICNT.enabled)
    {
        ROMCTRL.word_ready = false;
        
        if (ROMCTRL.block_size == 0)
            bytes_left = 0;
        else if (ROMCTRL.block_size == 7)
            bytes_left = 4;
        else
            bytes_left = 0x100 << ROMCTRL.block_size;
        
        if (cmd_encrypt_mode)
            cycles_left += ROMCTRL.key1_gap;
        if (cmd_encrypt_mode == 1)
        {
            uint8_t data[8];
            for (int i = 0; i < 8; i++)
                data[i] = command_buffer[7 - i];
            key1_decrypt((uint32_t*)data);
            
            printf("\nData sent: $%02X%02X%02X%02X%02X%02X%02X%02X", command_buffer[0], command_buffer[1], command_buffer[2], command_buffer[3], command_buffer[4], command_buffer[5], command_buffer[6], command_buffer[7]);
            printf("\nDecrypted: $%02X%02X%02X%02X%02X%02X%02X%02X", data[7], data[6], data[5], data[4], data[3], data[2], data[1], data[0]);
            for (int i = 0; i < 8; i++)
                command_buffer[7 - i] = data[i];
            
        }
        
        command_id = CART_COMMAND::EMPTY;
        
        switch (command_buffer[0])
        {
            case 0x9F:
                command_id = CART_COMMAND::DUMMY;
                break;
            case 0x00:
                command_id = CART_COMMAND::GET_HEADER;
                ROM_data_index = 0;
                break;
            case 0x90:
                command_id = CART_COMMAND::GET_CHIP_ID;
                break;
            case 0x3C:
                command_id = CART_COMMAND::ENABLE_KEY1;
                break;
            case 0xB7:
                command_id = CART_COMMAND::READ_ROM;
                ROM_data_index = (command_buffer[1] << 24);
                ROM_data_index |= (command_buffer[2] << 16);
                ROM_data_index |= (command_buffer[3] << 8);
                ROM_data_index |= (command_buffer[4]);
                if (bytes_left > 0x1000)
                    throw "[CART] ROM read bytes left > 0x1000";
                break;
            case 0xB8:
                command_id = CART_COMMAND::GET_CHIP_ID;
                break;
            default:
                switch (command_buffer[0] & 0xF0)
                {
                    case 0x40:
                        //Do nothing? According to docs key 2 encryption is handled by hardware
                        command_id = CART_COMMAND::DUMMY;
                        break;
                    case 0x10:
                        command_id = CART_COMMAND::GET_CHIP_ID;
                        break;
                    case 0x20:
                        command_id = CART_COMMAND::GET_SECURE_AREA_BLOCK;
                        
                        secure_area_index = (command_buffer[2] & 0xF0) << 8;
                        break;
                    case 0xA0:
                        command_id = CART_COMMAND::DUMMY;
                        cmd_encrypt_mode = 2;
                        break;
                }
        }
    }
}

void NDS_Cart::set_lo_key2_seed0(uint32_t word)
{
    encrypt_seed0 >>= 32;
    encrypt_seed0 <<= 32;
    encrypt_seed0 |= word;
}

void NDS_Cart::set_lo_key2_seed1(uint32_t word)
{
    encrypt_seed1 >>= 32;
    encrypt_seed1 <<= 32;
    encrypt_seed1 |= word;
}

void NDS_Cart::set_hi_key2_seed0(uint32_t word)
{
    word &= 0x7F;
    encrypt_seed0 <<= 32;
    encrypt_seed0 >>= 32;
    encrypt_seed0 |= static_cast<uint64_t>(word) << 32;
}

void NDS_Cart::set_hi_key2_seed1(uint32_t word)
{
    word &= 0x7F;
    encrypt_seed1 <<= 32;
    encrypt_seed1 >>= 32;
    encrypt_seed1 |= static_cast<uint64_t>(word) << 32;
}
