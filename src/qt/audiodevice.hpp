/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef AUDIODEVICE_HPP
#define AUDIODEVICE_HPP

#include <QIODevice>
#include "../spu.hpp"

class AudioDevice : public QIODevice
{
    Q_OBJECT
    private:
        SPU* spu;
        int16_t buffer[SPU::SAMPLE_BUFFER_SIZE];
        int buffer_size;
    public:
        explicit AudioDevice(QObject *parent = nullptr);
        qint64 readData(char *data, qint64 maxlen) override;
        qint64 writeData(const char *data, qint64 len) override;
        qint64 bytesAvailable() const override;

        void clear_buffer();

        void set_SPU(SPU* spu);
    signals:

    public slots:
};

#endif // AUDIODEVICE_HPP
