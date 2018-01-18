#ifndef SLOT2_HPP
#define SLOT2_HPP
#include <cstdint>

class Slot2Device
{
    private:
        uint8_t* memory;
        uint64_t mem_size;
    public:
        Slot2Device();
        ~Slot2Device();

        void load_data(uint8_t* data, uint64_t size);
        template <typename T> T read(uint32_t address);
        //template <typename T> void write(uint32_t address, T value);
};

template <typename T>
T Slot2Device::read(uint32_t address)
{
    address &= 0x1FFFFFF;
    if (address < mem_size)
    {
        return *(T*)&memory[address];
    }
    return (T)0xFFFFFFFF;
}

#endif // SLOT2_HPP
