/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <cstdio>
#include "audiodevice.hpp"

AudioDevice::AudioDevice(QObject *parent) : QIODevice(parent), spu(nullptr)
{
    clear_buffer();
}

qint64 AudioDevice::readData(char *data, qint64 maxlen)
{
    int samples = spu->output_buffer(buffer) * 2;
    if (samples)
        buffer_size = samples;
    if (buffer_size > maxlen)
    {
        printf("\nBuffer size exceeds maxlen: %d", maxlen);
        buffer_size = maxlen;
    }
    memcpy(data, buffer, buffer_size);
    //printf("\nSamples: %d", samples);
    return buffer_size;
}

qint64 AudioDevice::writeData(const char *data, qint64 len)
{
    return 0;
}

qint64 AudioDevice::bytesAvailable() const
{
    return spu->get_samples() * 2 + QIODevice::bytesAvailable();
}

void AudioDevice::clear_buffer()
{
    for (int i = 0; i < 64; i++)
        buffer[i] = 0;
    buffer_size = 64;
}

void AudioDevice::set_SPU(SPU *spu)
{
    this->spu = spu;
}
