/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <fstream>
#include "emulator.hpp"
#include "firmware.hpp"

using namespace std;

Firmware::Firmware(Emulator *e) : e(e) {}

int Firmware::load_firmware(string file_name)
{
    ifstream firmware_file(file_name, ios::in | ios::binary);
    if (!firmware_file.is_open())
        return 1;
    
    //TODO: bounds checking
    firmware_file.read((char*)firmware, SIZE);
    
    firmware_file.close();

    user_data = 0x3FE00;

    if (*(uint16_t*)&firmware[user_data + 0x170] == ((*(uint16_t*)&firmware[user_data + 0x70] + 1) & 0x7F))
    {
        if (verify_CRC(0xFFFF, user_data + 0x100, 0x70, user_data + 0x172))
            user_data += 0x100;
    }

    *(uint16_t*)&firmware[user_data+0x58] = 0;
    *(uint16_t*)&firmware[user_data+0x5A] = 0;
    firmware[user_data+0x5C] = 0;
    firmware[user_data+0x5D] = 0;
    *(uint16_t*)&firmware[user_data+0x5E] = 255 << 4;
    *(uint16_t*)&firmware[user_data+0x60] = 191 << 4;
    firmware[user_data+0x62] = 255;
    firmware[user_data+0x63] = 191;

    *(uint16_t*)&firmware[user_data+0x72] = create_CRC(&firmware[user_data], 0x70, 0xFFFF);

    *(uint16_t*)&firmware[0x2A] = create_CRC(&firmware[0x2C], *(uint16_t*)&firmware[0x2C], 0x0000);

    printf("\nFW: USER0 CRC16 = %s", verify_CRC(0xFFFF, 0x3FE00, 0x70, 0x3FE72)?"GOOD":"BAD");
    printf("\nFW: USER1 CRC16 = %s", verify_CRC(0xFFFF, 0x3FF00, 0x70, 0x3FF72)?"GOOD":"BAD");
    
    command_id = FIRM_COMMAND::NONE;
    status_reg = 0;
    return 0;
}

uint16_t Firmware::create_CRC(uint8_t *data, int length, uint32_t start)
{
    uint16_t stuff[8] = {0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001, 0xA001};
    for (int i = 0; i < length; i++)
    {
        start ^= data[i];

        for (int j = 0; j < 8; j++)
        {
            if (start & 0x1)
            {
                start >>= 1;
                start ^= (stuff[j] << (7 - j));
            }
            else
                start >>= 1;
        }
    }
    return start & 0xFFFF;
}

bool Firmware::verify_CRC(uint32_t start, uint32_t offset, uint32_t length, uint32_t CRC_offset)
{
    uint16_t stored_CRC = *(uint16_t*)&firmware[CRC_offset];
    uint16_t calculated_CRC = create_CRC(&firmware[offset], length, start);
    printf("\nStored CRC: $%04X", stored_CRC);
    printf("\nCalc CRC: $%04X", calculated_CRC);
    return stored_CRC == calculated_CRC;
}

void Firmware::direct_boot()
{
    e->arm7_write_word(0x27FF864, 0);
    e->arm7_write_word(0x27FF868, *(uint16_t*)&firmware[0x20] << 3);

    e->arm7_write_halfword(0x027FF874, *(uint16_t*)&firmware[0x26]);
    e->arm7_write_halfword(0x027FF876, *(uint16_t*)&firmware[0x04]);

    for (int i = 0; i < 0x70; i += 4)
        e->arm7_write_word(0x027FFC80+i, *(uint32_t*)&firmware[user_data+i]);
}

uint8_t Firmware::transfer_data(uint8_t input)
{
    total_args++;
    switch (command_id)
    {
        case FIRM_COMMAND::READ_STREAM:
            if (total_args < 5)
            {
                address <<= 8;
                address |= input;
            }
            else
            {
                address++;
                //printf("Firmware $%08X: $%02X\n", address - 1, firmware[(address - 1) & (SIZE - 1)]);
                return firmware[(address - 1) & (SIZE - 1)];
            }
            break;
        case FIRM_COMMAND::READ_STATUS_REG:
            return status_reg;
        default: //Interpret new command
            switch (input)
            {
                case 0x03:
                    command_id = FIRM_COMMAND::READ_STREAM;
                    break;
                case 0x04: //Disable writes
                    status_reg &= ~0x1;
                    break;
                case 0x05:
                    command_id = FIRM_COMMAND::READ_STATUS_REG;
                    break;
                case 0x06: //Enable writes
                    status_reg |= 0x1;
                    break;
                default:
                    printf("Command $%02X not recognized in firmware", input);
                    throw "[FIRMWARE] Unrecognized command";
            }
            break;
    }
    
    return input;
}

void Firmware::deselect()
{
    command_id = FIRM_COMMAND::NONE;
    address = 0;
    total_args = 0;
}
