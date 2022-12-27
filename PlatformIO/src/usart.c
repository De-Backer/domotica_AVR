#include "../include/usart.h"

#include <stdlib.h>

static void init_USART0_baud_rate_2500000bps()
{
    /* set double speed mode */
    UCSR0A = (1 << U2X0);
    /* set the baud rate */
    UBRR0H = 0;
    UBRR0L = 0;
}
static void init_USART0_baud_rate_1250000bps()
{
    /* set the baud rate */
    UBRR0H = 0;
    UBRR0L = 0;
}
static void init_USART0_baud_rate_250000bps()
{
    /* set the baud rate */
    UBRR0H = 0;
    UBRR0L = 4;
}
static void init_USART0_baud_rate_9600bps()
{
    /* set the baud rate */
    UBRR0H = 0;
    UBRR0L = 129;
}
static void init_USART0_baud_rate_2400bps()
{
    /* set the baud rate */
    UBRR0H = 2;
    UBRR0L = 8;
}
/** \brief init_de USART0
 * Configuring USART0
 * Frame format :
 * - One start bit
 * - 8 data bits
 * - No parity bit
 * - one stop bits
 * baud rate: 2500000bps
 * 0% Error
 **/
inline void init_USART0()
{
    init_USART0_baud_rate_2500000bps();

    /* Enable  transmitter */
    /* Enable receiver */
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    /* Set frame format: 8data, 1 stop bit */
    UCSR0C = (3 << UCSZ00);
    RingBuffer_InitBuffer(&RX_Buffer, RX_BufferData, sizeof(RX_BufferData));
}

/** \brief zend_USART0
 * */
inline void Transmit_USART0(unsigned char data)
{
    /* Wait for empty transmit buffer */
    while (! (UCSR0A & (1 << UDRE0)))
        ;
    /* Put data into buffer, sends the data */
    UDR0 = data;
}

/**
 * @brief Test_Transmit_USART0
 * zent a test data
 */
void Test_Transmit_USART0()
{
    char* Buffer = "Test USART0";
    Transmit_USART0(10); /* new line */
    Transmit_USART0(10); /* new line */

    while (*Buffer) { Transmit_USART0(*Buffer++); }

    Transmit_USART0(10); /* new line */
    Transmit_USART0(10); /* new line */
}

void Receive_USART0()
{
    if (UCSR0A & (1 << RXC0))
    {
        unsigned char data = UDR0;
        if (RingBuffer_IsFull(&RX_Buffer))
        {
            /* Buffer Is Full */

            char* Buffer = "ERROR RX Buffer ";

            Transmit_USART0(10); /* new line */
            Transmit_USART0(10); /* new line */

            while (*Buffer) { Transmit_USART0(*Buffer++); }

            Transmit_USART0(data);

            Transmit_USART0(10); /* new line */
            Transmit_USART0(10); /* new line */

            return;
        }

        RingBuffer_Insert(&RX_Buffer, data);
    }
}
