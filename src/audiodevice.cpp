#include "audiodevice.hpp"

AudioDevice::AudioDevice(QObject *parent) : QIODevice(parent), spu(nullptr)
{

}

qint64 AudioDevice::readData(char *data, qint64 maxlen)
{
    return spu->output_buffer((int16_t*)data) * 2;
}

qint64 AudioDevice::writeData(const char *data, qint64 len)
{
    return 0;
}

qint64 AudioDevice::bytesAvailable() const
{
    return spu->get_samples() * 2 + QIODevice::bytesAvailable();
}

void AudioDevice::set_SPU(SPU *spu)
{
    this->spu = spu;
}
