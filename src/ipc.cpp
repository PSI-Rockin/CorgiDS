/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "ipc.hpp"
#include <cstdlib>

using namespace std;

void clear_queue(queue<uint32_t>& q)
{
    queue<uint32_t> empty;
    swap(q, empty);
}

IPCSYNC::IPCSYNC() : input(0), output(0), IRQ_enable(false) {}

uint16_t IPCSYNC::read()
{
    uint16_t halfword = 0;
    halfword |= input;
    halfword |= output << 8;
    halfword |= IRQ_enable << 14;
    return halfword;
}

void IPCSYNC::receive_input(uint16_t halfword)
{
    input = (halfword >> 8) & 0xF;
}

void IPCSYNC::write(uint16_t halfword)
{
    output = (halfword >> 8) & 0xF;
    IRQ_enable = halfword & (1 << 14);
}

uint16_t IPCFIFO::read_CNT()
{
    uint16_t reg = 0;
    reg |= (send_queue->size() == 0); //Empty status
    reg |= (send_queue->size() == 16) << 1; //Full status
    reg |= send_empty_IRQ << 2;
    reg |= (receive_queue->size() == 0) << 8;
    reg |= (receive_queue->size() == 16) << 9;
    reg |= receive_nempty_IRQ << 10;
    reg |= error << 14;
    reg |= enabled << 15;
    return reg;
}

void IPCFIFO::write_CNT(uint16_t halfword)
{
    if (!send_empty_IRQ && (halfword & (1 << 2)) && send_queue->size() == 0)
        request_empty_IRQ = true;
    if (!receive_nempty_IRQ && (halfword & (1 << 10)) && receive_queue->size())
        request_nempty_IRQ = true;
    send_empty_IRQ = halfword & (1 << 2);
    receive_nempty_IRQ = halfword & (1 << 10);
    error &= ~(halfword & (1 << 14));
    enabled = halfword & (1 << 15);

    if (halfword & (1 << 3))
        clear_queue(*send_queue);
}

uint32_t IPCFIFO::read_queue()
{
    if (!enabled)
        return recent_word;
    if (send_queue->size())
    {
        uint32_t word = send_queue->front();
        send_queue->pop();
        if (!send_queue->size() && send_empty_IRQ)
            request_empty_IRQ = true;
        return word;
    }
    error = true;
    return recent_word;
}

void IPCFIFO::write_queue(uint32_t word)
{
    if (!enabled)
        return;
    if (receive_queue->size() >= 16)
        error = true;
    else
    {
        if (!receive_queue->size() && receive_nempty_IRQ)
            request_nempty_IRQ = true;
        receive_queue->push(word);
    }
}
