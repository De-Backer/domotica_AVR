
#ifndef _usart_h__
#define _usart_h__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../include/RingBuffer.h"

#include <avr/io.h>
#include <util/delay.h>
    void init_USART0();

    void Transmit_USART0(unsigned char data);
    void         Test_Transmit_USART0();
    void         Receive_USART0();
    RingBuffer_t RX_Buffer;
    uint8_t      RX_BufferData[20];

#    ifdef __cplusplus
} // extern "C"
#endif

#endif
