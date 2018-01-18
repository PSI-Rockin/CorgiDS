#include <cstring>
#include "slot2.hpp"

Slot2Device::Slot2Device() : memory(nullptr), mem_size(0)
{

}

Slot2Device::~Slot2Device()
{
    if (memory)
        delete[] memory;
}

void Slot2Device::load_data(uint8_t *data, uint64_t size)
{
    if (size)
    {
        if (memory)
            delete[] memory;
        memory = new uint8_t[size];
        memcpy(memory, data, size);
        mem_size = size;
    }
}
