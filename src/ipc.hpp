/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef IPC_HPP
#define IPC_HPP
#include <cstdint>
#include <queue>

struct IPCSYNC
{
    int input;
    int output;
    bool IRQ_enable;

    IPCSYNC();
    uint16_t read();
    void receive_input(uint16_t halfword);
    void write(uint16_t halfword);
};

struct IPCFIFO
{
    std::queue<uint32_t> *send_queue, *receive_queue;
    uint32_t recent_word;

    bool send_empty_IRQ;
    bool request_empty_IRQ;
    bool receive_nempty_IRQ;
    bool request_nempty_IRQ;
    bool error;
    bool enabled;

    uint16_t read_CNT();
    void write_CNT(uint16_t value);

    uint32_t read_queue();
    void write_queue(uint32_t word);
};

#endif // IPC_H
