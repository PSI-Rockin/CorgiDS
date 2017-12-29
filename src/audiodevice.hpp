#ifndef AUDIODEVICE_HPP
#define AUDIODEVICE_HPP

#include <QIODevice>
#include "spu.hpp"

class AudioDevice : public QIODevice
{
    Q_OBJECT
    private:
        SPU* spu;
    public:
        explicit AudioDevice(QObject *parent = nullptr);
        qint64 readData(char *data, qint64 maxlen) override;
        qint64 writeData(const char *data, qint64 len) override;
        qint64 bytesAvailable() const override;

        void set_SPU(SPU* spu);
    signals:

    public slots:
};

#endif // AUDIODEVICE_HPP
