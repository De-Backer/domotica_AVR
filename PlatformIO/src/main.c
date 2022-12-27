/* main.c
 * 20200330
 * Author: Simon De Backer
 * info  :
 * atmega1284p ADC naar USART 8 ingangen
 *
 * 5V
 * - 10K NTC
 * ------ Pin ADC
 * - 2K4 weerstand
 * GND
 */

/* Intel_HEX info
 * : BB AAAA RR DDDD DDDD DDDD DDDD DDDD DDDD DDDD DDDD CC
 *
 * :    start code
 * BB   Byte count (meestal is dit 10 => 16 Bytes)
 * AAAA Address
 * RR   Record type
 *      - 00 Data
 *      - 01 End of File
 * DD   Data (*16)
 * CC   Checksum
 *      als je alles optelt van "Byte count" tot en met "Checksum"
 *      moeten de LSB 00 zijn
 *      eg: 00 + 01 + FF = 100 => x 00 is ok
 */
#ifndef _main_c__
#define _main_c__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../include/CAN_MCP2515.h"
#include "../include/SPI.h"
#include "../include/usart.h"

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>

/* Debug */
#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)
#define DEBUG        __FILE__ ":" TOSTRING(__LINE__)

//uint8_t mcusr __attribute__ ((section (".noinit")));//<= the MCU Status Register
//void getMCUSR(void) __attribute__((naked)) __attribute__((section(".init0")));
//void getMCUSR(void)
//{
//    __asm__ __volatile__ ( "mov %0, r2 \n" : "=r" (mcusr) : );
//}

/* CAN to EEPROM
 * config        CAN_Priority_config
 * module adres  0x00 - 0xff
 * Frame and rtr 0x00
 * length        0x05 - 0x08
 * data EEPROM   0x01
 * data EEPROM   0x01 (read) 0x02 (update & read)
 * data adres_H  0x00 - 0x0f <---(max 4Kbit)
 * data adres_L  0x00 - 0xff
 * data 0        0x00 - 0xff
 * data 1        0x00 - 0xff | see length 6
 * data 2        0x00 - 0xff |            7
 * data 3        0x00 - 0xff |            8
 *
 *  voorstel:
 *  reset module om de "echo_id_Adres();" te starten
 *  dan weet men de:
 *  -microcontroller_id
 *  -module_adres         => module_adres
 *  -MICROCONTROLLER_TYPE
 *  -PROTOCOL_VERSIE
 *  -EE_IO_block          => de ofset in de eeprom
 *  -I_max_block
 *  -O_max_block
 */
/* CAN_Priority_config [module_adres] 0x00 0x00 0x06
 *                                    0x01 0x02 0x00 0x00 <-- EE_MICROCONTROLLER_ID
 *                microcontroller_id_H
 *                microcontroller_id_L
 *
 * CAN_Priority_config [module_adres] 0x00 0x00 0x05
 *                               0x01 0x02 0x00 0x02 <-- EE_MODULE_ADRES
 *                module_adres
 */

/* EEPROM List of parameters */
#define EE_MICROCONTROLLER_ID 0 /* 16 bit */
#define EE_MODULE_ADRES       2

    static uint16_t microcontroller_id = 0x0000; // default
    static uint8_t  module_adres       = 0x00;   // default
    /* ?? => microcontroller_id == 0x0000
     *  yes can stuurt naar id 0 van id 0
     *  de µc die stuurt luistert niet naar zijn eigen bericht!
     * maar niet met 3 µc met id 0 beginnen.
     */

#if defined(__AVR_ATmega1284P__)

#    define MICROCONTROLLER_TYPE 0x01
#    define PROTOCOL_VERSIE      0x02 /* Mod adc + Fix set out to can */

/* 0x04 [module_adres] 0x00 0x00 0x08
 *                0x01 0x02 0x00 0x03 <-- EE_MICROCONTROLLER_DDRA
 *                DDRA DDRB DDRC DDRD
 *
 *          0x01  0xff  0x00  0x00 0x00
 *          0x02  0x00  0x00  0x00 0x00
 */
#    define EE_MICROCONTROLLER_DDRA 3
#    define EE_MICROCONTROLLER_DDRB 4
#    define EE_MICROCONTROLLER_DDRC 5
#    define EE_MICROCONTROLLER_DDRD 6

/* 0x04 [module_adres]  0x00  0x00  0x08
 *                0x01  0x02  0x00  0x07 <-- EE_MICROCONTROLLER_PORTA
 *                PORTA PORTB PORTC PORTD
 *
 *          0x01  0xff  0x00  0x00 0x00
 *          0x02  0x00  0x00  0x00 0x00
 */
#    define EE_MICROCONTROLLER_PORTA 7
#    define EE_MICROCONTROLLER_PORTB 8
#    define EE_MICROCONTROLLER_PORTC 9
#    define EE_MICROCONTROLLER_PORTD 10
/* Pinout ATmega1284P
 *        | \__/ |
 * 08 PB0-| 1  40|-PA0 00
 * 09 PB1-| 2  39|-PA1 01
 * 0a PB2-| 3  38|-PA2 02
 * 0b PB3-| 4  37|-PA3 03
 * 0c PB4-| 5  36|-PA4 04
 * 0d PB5-| 6  35|-PA5 05
 * 0e PB6-| 7  34|-PA6 06
 * 0f PB7-| 8  33|-PA7 07
 * Reset -| 9  32|-Aref
 * VCC   -|10  31|-GND
 * GND   -|11  30|-AVCC
 * XTAL2 -|12  29|-PC7 17
 * XTAL1 -|13  28|-PC6 16
 * 18 PD0-|14  27|-PC5 15
 * 19 PD1-|15  26|-PC4 14
 * 1a PD2-|16  25|-PC3 13
 * 1b PD3-|17  24|-PC2 12
 * 1c PD4-|18  23|-PC1 11
 * 1d PD5-|19  22|-PC0 10
 * 1e PD6-|20  21|-PD7 1f
*/
#    define EE_EXTENDED_DDR0A 11
#    define EE_EXTENDED_DDR0B 12
#    define EE_EXTENDED_DDR1A 13
#    define EE_EXTENDED_DDR1B 14
#    define EE_EXTENDED_DDR2A 15
#    define EE_EXTENDED_DDR2B 16
#    define EE_EXTENDED_DDR3A 17
#    define EE_EXTENDED_DDR3B 18
#    define EE_EXTENDED_DDR4A 19
#    define EE_EXTENDED_DDR4B 20
#    define EE_EXTENDED_DDR5A 21
#    define EE_EXTENDED_DDR5B 22
#    define EE_EXTENDED_DDR6A 23
#    define EE_EXTENDED_DDR6B 24
#    define EE_EXTENDED_DDR7A 25
#    define EE_EXTENDED_DDR7B 26
#    define EE_EXTENDED_DDR8A 27
#    define EE_EXTENDED_DDR8B 28

#    define EE_EXTENDED_PORT_0A 29
#    define EE_EXTENDED_PORT_0B 30
#    define EE_EXTENDED_PORT_1A 31
#    define EE_EXTENDED_PORT_1B 32
#    define EE_EXTENDED_PORT_2A 34
#    define EE_EXTENDED_PORT_2B 35
#    define EE_EXTENDED_PORT_3A 36
#    define EE_EXTENDED_PORT_3B 37
#    define EE_EXTENDED_PORT_4A 38
#    define EE_EXTENDED_PORT_4B 39
#    define EE_EXTENDED_PORT_5A 40
#    define EE_EXTENDED_PORT_5B 41
#    define EE_EXTENDED_PORT_6A 42
#    define EE_EXTENDED_PORT_6B 43
#    define EE_EXTENDED_PORT_7A 44
#    define EE_EXTENDED_PORT_7B 45
#    define EE_EXTENDED_PORT_8A 46
#    define EE_EXTENDED_PORT_8B 47
    /* de all_of functie
     * - masker die de uitgangen "all off" bepaald
     * - toestand waardat de uitgang moet zijn (0 of 1)
     * bv 1:
     * PortA |=(EE_all_of_E_MICROCONTROLLER_PORTA && EE_all_of_MICROCONTROLLER_PORTA);
     * bv 0:
     * PortA &=(EE_all_of_E_MICROCONTROLLER_PORTA && ~EE_all_of_MICROCONTROLLER_PORTA);
     *
     */
#    define EE_all_of_E_MICROCONTROLLER_PORTA 48
#    define EE_all_of_E_MICROCONTROLLER_PORTB 49
#    define EE_all_of_E_MICROCONTROLLER_PORTC 50
#    define EE_all_of_E_MICROCONTROLLER_PORTD 51
#    define EE_all_of_E_EXTENDED_PORT_0A 52
#    define EE_all_of_E_EXTENDED_PORT_0B 53
#    define EE_all_of_E_EXTENDED_PORT_1A 54
#    define EE_all_of_E_EXTENDED_PORT_1B 55
#    define EE_all_of_E_EXTENDED_PORT_2A 56
#    define EE_all_of_E_EXTENDED_PORT_2B 57
#    define EE_all_of_E_EXTENDED_PORT_3A 58
#    define EE_all_of_E_EXTENDED_PORT_3B 59
#    define EE_all_of_E_EXTENDED_PORT_4A 60
#    define EE_all_of_E_EXTENDED_PORT_4B 61
#    define EE_all_of_E_EXTENDED_PORT_5A 62
#    define EE_all_of_E_EXTENDED_PORT_5B 63
#    define EE_all_of_E_EXTENDED_PORT_6A 64
#    define EE_all_of_E_EXTENDED_PORT_6B 65
#    define EE_all_of_E_EXTENDED_PORT_7A 66
#    define EE_all_of_E_EXTENDED_PORT_7B 67
#    define EE_all_of_E_EXTENDED_PORT_8A 68
#    define EE_all_of_E_EXTENDED_PORT_8B 69

#    define EE_all_of_MICROCONTROLLER_PORTA 70
#    define EE_all_of_MICROCONTROLLER_PORTB 71
#    define EE_all_of_MICROCONTROLLER_PORTC 72
#    define EE_all_of_MICROCONTROLLER_PORTD 73
#    define EE_all_of_EXTENDED_PORT_0A 74
#    define EE_all_of_EXTENDED_PORT_0B 75
#    define EE_all_of_EXTENDED_PORT_1A 76
#    define EE_all_of_EXTENDED_PORT_1B 77
#    define EE_all_of_EXTENDED_PORT_2A 78
#    define EE_all_of_EXTENDED_PORT_2B 79
#    define EE_all_of_EXTENDED_PORT_3A 80
#    define EE_all_of_EXTENDED_PORT_3B 81
#    define EE_all_of_EXTENDED_PORT_4A 82
#    define EE_all_of_EXTENDED_PORT_4B 83
#    define EE_all_of_EXTENDED_PORT_5A 84
#    define EE_all_of_EXTENDED_PORT_5B 85
#    define EE_all_of_EXTENDED_PORT_6A 86
#    define EE_all_of_EXTENDED_PORT_6B 87
#    define EE_all_of_EXTENDED_PORT_7A 88
#    define EE_all_of_EXTENDED_PORT_7B 89
#    define EE_all_of_EXTENDED_PORT_8A 90
#    define EE_all_of_EXTENDED_PORT_8B 91

    /* Start reset => new IO laden
     * CAN_Priority_config [module_adres]  0x00  0x00  0x02 0x04 0x04
     */

    /* CAN to out:
     * aantal inputs kopelen aan output
     *  | Adres
     *  | data 0 command
     *  | data 1 number
     *  | data 2 toestand
     *  | naam_output
     *  |== 5 byte
     *  => [Adres] [command] [number] [toestand] [naam_output]
     *      0x01     0x01      0x05      0x01       0x10
     *
     *  +output doet wat?
     *  |naam_output
     *  |function (aan/uit/pwm/...)
     *  |data
     *  == 4 byte
     *  => [naam_output] [function] [PIN-number] [data]
     *         0x10        0x02       0x00-0xff  0x00-0xff
     *
     *         function
     *         - 00 uitgang uit
     *         - 01 uitgang aan
     *         - 02 uitgang togel
     *         - 03 PWM-uitgang
     *         - 04 DAC-uitgang
     *
     **/

#    define EE_IO_block 100 // 0x64
#    define I_max_block 0xff // 0x64 + I_max_block * 5 = ofset for O_from_EEPROM
#    define O_max_block 0xff
    static uint8_t I_from_EEPROM[I_max_block][5]; // [0--255] [Adres / command / number
                                           // / toestand / naam_output]
    static uint8_t O_from_EEPROM[O_max_block]
                         [3]; // [naam_output] [function / PIN-number / data]
    static uint8_t current_O[O_max_block][2];// [PIN-number] [function / data]
#else
#    warning "device type not defined"
#endif

#if PROTOCOL_VERSIE==0x00
#   define CAN_Priority_High_reserve 0x000
#   define CAN_Priority_global       0x100
#   define CAN_Priority_High         0x200
#   define CAN_Priority_USART1       0x300
#   define CAN_Priority_config       0x400
#   define CAN_Priority_set          0x500
#   define CAN_Priority_normale      0x600
#   define CAN_Priority_reserve      0x700
#elif PROTOCOL_VERSIE==0x01
#   define CAN_Priority_High_reserve 0x000
#   define CAN_Priority_global       0x100
#   define CAN_Priority_High         0x200
#   define CAN_Priority_High_USART1  0x300
#   define CAN_Priority_set          0x400
#   define CAN_Priority_normale      0x500
#   define CAN_Priority_USART1       0x600
#   define CAN_Priority_config       0x700
#elif PROTOCOL_VERSIE==0x02
#   define CAN_Priority_High_reserve 0x000
#   define CAN_Priority_global       0x100
#   define CAN_Priority_High         0x200
#   define CAN_Priority_High_USART1  0x300
#   define CAN_Priority_set          0x400
#   define CAN_Priority_normale      0x500
#   define CAN_Priority_USART1       0x600
#   define CAN_Priority_config       0x700
#else
#    warning "PROTOCOL_VERSIE not defined"
#endif

    /* Configuring the Pin */
    static void init_io()
    {
        if (module_adres == 0xff)
        {
            /* no adres => all tri-state */

            /* Data Direction Register
             * 0 = ingang
             * 1 = uitgang
             **/
            DDRA = 0x00;
            DDRB = 0x00;
            DDRC = 0x00;
            DDRD = 0x00;
            /* Data Register
             * 0 = laag (uitgang) / tri-state (ingang)
             * 1 = hoog (uitgang) / pull up (ingang)
             **/
            PORTA = 0x00;
            PORTB = 0x00;
            PORTC = 0x00;
            PORTD = 0x00;
            return;
        }
        DDRA = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_DDRA);
        DDRB = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_DDRB);
        DDRC = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_DDRC);
        DDRD = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_DDRD);

        PORTA = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_PORTA);
        PORTB = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_PORTB);
        PORTC = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_PORTC);
        PORTD = eeprom_read_byte((uint8_t*) EE_MICROCONTROLLER_PORTD);
    }

    static void CAN_echo_id_Adres(uint8_t data1, uint8_t data2)
    {
        /* info Microcontroller can */

        //                         0x1FFFFFFF | 0xFFFF max 29-bit
        CAN_TX_msg.id           = (0x01000000 | microcontroller_id);
        CAN_TX_msg.ext_id       = CAN_EXTENDED_FRAME;
        CAN_TX_msg.rtr          = 0;
        CAN_TX_msg.length       = 8;
        CAN_TX_msg.data_byte[0] = MICROCONTROLLER_TYPE;
        CAN_TX_msg.data_byte[1] = PROTOCOL_VERSIE;
        CAN_TX_msg.data_byte[2] = module_adres;
        CAN_TX_msg.data_byte[3] = EE_IO_block;
        CAN_TX_msg.data_byte[4] = I_max_block;
        CAN_TX_msg.data_byte[5] = O_max_block;
        CAN_TX_msg.data_byte[6] = data1;
        CAN_TX_msg.data_byte[7] = data2;

        MCP2515_message_TX();

        CAN_TX_msg.id           = (0x02000000 | microcontroller_id);
        CAN_TX_msg.ext_id       = CAN_EXTENDED_FRAME;
        CAN_TX_msg.rtr          = 0;
        CAN_TX_msg.length       = 8;
        CAN_TX_msg.data_byte[0] = GIT_COMMIT_SHA[0];
        CAN_TX_msg.data_byte[1] = GIT_COMMIT_SHA[1];
        CAN_TX_msg.data_byte[2] = GIT_COMMIT_SHA[2];
        CAN_TX_msg.data_byte[3] = GIT_COMMIT_SHA[3];
        CAN_TX_msg.data_byte[4] = GIT_COMMIT_SHA[4];
        CAN_TX_msg.data_byte[5] = GIT_COMMIT_SHA[5];
        CAN_TX_msg.data_byte[6] = GIT_COMMIT_SHA[6];
        CAN_TX_msg.data_byte[7] = GIT_COMMIT_SHA[7];

        MCP2515_message_TX();
    }

    static void USART_echo_id_Adres(uint8_t data1, uint8_t data2)
    {
        /* info Microcontroller can */

        typedef union
        {
            uint32_t long_id;
            uint8_t  int_id[8];
        } ID;
        ID id;
        id.long_id = (0x01000000 | microcontroller_id);
        Transmit_USART0(10); /* new line */
        Transmit_USART0(id.int_id[3]);
        Transmit_USART0(id.int_id[2]);
        Transmit_USART0(id.int_id[1]);
        Transmit_USART0(id.int_id[0]);
        Transmit_USART0(CAN_EXTENDED_FRAME);
        Transmit_USART0(0x00);
        Transmit_USART0(0x08);
        Transmit_USART0(MICROCONTROLLER_TYPE);
        Transmit_USART0(PROTOCOL_VERSIE);
        Transmit_USART0(module_adres);
        Transmit_USART0(EE_IO_block);
        Transmit_USART0(I_max_block);
        Transmit_USART0(O_max_block);
        Transmit_USART0(data1);
        Transmit_USART0(data2);
    }

    static void set_port(uint8_t uitgang, uint8_t state, uint8_t duur)
    {
        CAN_TX_msg.id           = CAN_Priority_normale | module_adres;
        CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
        CAN_TX_msg.rtr          = 0;
        CAN_TX_msg.length       = 4;
        CAN_TX_msg.data_byte[0] = 0x03; /* 1 uitgang */
        CAN_TX_msg.data_byte[1] = uitgang;
        CAN_TX_msg.data_byte[2] = state;
        CAN_TX_msg.data_byte[3] = duur;
        CAN_TX_msg.data_byte[4] = 0;
        CAN_TX_msg.data_byte[5] = 0;
        CAN_TX_msg.data_byte[6] = 0;
        CAN_TX_msg.data_byte[7] = 0;

        //set current pin state and duur
        current_O[uitgang][0]=state;
        current_O[uitgang][1]=duur;

        if (uitgang < 0x08) /* PORT A */
        {
            if (state == 0x01) { PORTA |= (1 << uitgang); }
            else if (state == 0x02)
            {
                PORTA ^= (1 << uitgang);
                CAN_TX_msg.data_byte[2] = (PORTA & (1 << uitgang));
            }
            else
            {
                PORTA &= ~(1 << uitgang);
            }
        }
        else if (uitgang < 0x10) /* PORT B */
        {
            uitgang -= 0x08;
            if (state == 0x01) { PORTB |= (1 << uitgang); }
            else if (state == 0x02)
            {
                PORTB ^= (1 << uitgang);
                CAN_TX_msg.data_byte[2] = (PORTB & (1 << uitgang));
            }
            else
            {
                PORTB &= ~(1 << uitgang);
            }
        }
        else if (uitgang < 0x18) /* PORT C */
        {
            uitgang -= 0x10;
            if (state == 0x01) { PORTC |= (1 << uitgang); }
            else if (state == 0x02)
            {
                PORTC ^= (1 << uitgang);
                CAN_TX_msg.data_byte[2] = (PORTC & (1 << uitgang));
            }
            else
            {
                PORTC &= ~(1 << uitgang);
            }
        }
        else if (uitgang < 0x20) /* PORT D */
        {
            uitgang -= 0x18;
            if (state == 0x01) { PORTD |= (1 << uitgang); }
            else if (state == 0x02)
            {
                PORTD ^= (1 << uitgang);
                CAN_TX_msg.data_byte[2] = (PORTD & (1 << uitgang));
            }
            else
            {
                PORTD &= ~(1 << uitgang);
            }
        }
        else
        {
            /* error */
            CAN_TX_msg.length       = 4;
            CAN_TX_msg.data_byte[0] = 0x00; /* error in code µc */
            CAN_TX_msg.data_byte[1] = 0x03; /* 1 uitgang */
            CAN_TX_msg.data_byte[2] = uitgang;
            CAN_TX_msg.data_byte[3] = state;
            CAN_TX_msg.data_byte[4] = duur;
        }
        while (MCP2515_message_TX()==0) {
            //0==Error no Transmit buffer empty
        }

    }

    static void build_can_block()
    {
        if (RingBuffer_Peek(&RX_Buffer) == 0x0A) /* new line */
        {
            RingBuffer_Remove(&RX_Buffer);                 /* new line */
            CAN_TX_msg.id = RingBuffer_Remove(&RX_Buffer); /* CAN ID */
            CAN_TX_msg.id = (CAN_TX_msg.id << 8);
            CAN_TX_msg.id |= RingBuffer_Remove(&RX_Buffer); /* CAN ID */
            CAN_TX_msg.id = (CAN_TX_msg.id << 8);
            CAN_TX_msg.id |= RingBuffer_Remove(&RX_Buffer); /* CAN ID */
            CAN_TX_msg.id = (CAN_TX_msg.id << 8);
            CAN_TX_msg.id |= RingBuffer_Remove(&RX_Buffer); /* CAN ID */
            CAN_TX_msg.ext_id       = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.rtr          = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.length       = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[0] = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[1] = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[2] = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[3] = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[4] = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[5] = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[6] = RingBuffer_Remove(&RX_Buffer);
            CAN_TX_msg.data_byte[7] = RingBuffer_Remove(&RX_Buffer);

            MCP2515_message_TX();
        }
        else
        {
            RingBuffer_Remove(&RX_Buffer);
        }
    }

    static void build_RAM_IO_from_EEPROM()
    {
        wdt_disable(); /* Stop Watchdog Reset */

        uint16_t ee_adres = EE_IO_block;
        uint8_t  temp[5];
        uint8_t  var = 0;
        for (; var < I_max_block; ++var)
        {
            eeprom_read_block((void*) temp, (const void*) ee_adres, 5);
            I_from_EEPROM[var][0] = temp[0]; // Adres
            I_from_EEPROM[var][1] = temp[1]; // command
            I_from_EEPROM[var][2] = temp[2]; // number
            I_from_EEPROM[var][3] = temp[3]; // toestand
            I_from_EEPROM[var][4] = temp[4]; // naam_output
            ee_adres += 5;
        }
        var = 0;
        for (; var < O_max_block; ++var)
        {
            eeprom_read_block((void*) temp, (const void*) ee_adres, 3);
            O_from_EEPROM[var][0] = temp[0]; // function
            O_from_EEPROM[var][1] = temp[1]; // number
            O_from_EEPROM[var][2] = temp[2]; // data
            ee_adres += 3;
        }

        wdt_enable(WDTO_250MS); /* Watchdog Reset after 250mSec */
    }

    static void CAN_EEPROM()
    {
        //        Transmit_USART0(10); /* new line */
        //        char* Buffer = "- EEPROM -";
        //        while (*Buffer) { Transmit_USART0(*Buffer++); }
        if (CAN_RX_msg.data_byte[1] == 0x01) /* read */
        {
            //            Transmit_USART0(10); /* new line */
            //            char* Buffer = "- read -";
            //            while (*Buffer) { Transmit_USART0(*Buffer++); }

            uint8_t length = CAN_RX_msg.length;
            length -= 4;
            uint16_t ee_adres;
            ee_adres = (CAN_RX_msg.data_byte[2] << 8);
            ee_adres |= CAN_RX_msg.data_byte[3];
            uint8_t temp[4];
            temp[0] = 0;
            temp[1] = 0;
            temp[2] = 0;
            temp[3] = 0;
            eeprom_read_block((void*) temp, (const void*) ee_adres, length);
            CAN_TX_msg.id           = (CAN_Priority_config | module_adres);
            CAN_TX_msg.ext_id       = 0;
            CAN_TX_msg.rtr          = 0;
            CAN_TX_msg.length       = length + 4;
            CAN_TX_msg.data_byte[0] = 0x01;
            CAN_TX_msg.data_byte[1] = 0x03;
            CAN_TX_msg.data_byte[2] = CAN_RX_msg.data_byte[2];
            CAN_TX_msg.data_byte[3] = CAN_RX_msg.data_byte[3];
            CAN_TX_msg.data_byte[4] = temp[0];
            CAN_TX_msg.data_byte[5] = temp[1];
            CAN_TX_msg.data_byte[6] = temp[2];
            CAN_TX_msg.data_byte[7] = temp[3];
            MCP2515_message_TX();
        }
        else if (CAN_RX_msg.data_byte[1] == 0x02) /* update & read */
        {
            //            Transmit_USART0(10); /* new line */
            //            char* Buffer = "- update -";
            //            while (*Buffer) { Transmit_USART0(*Buffer++); }

            uint8_t length = CAN_RX_msg.length;
            length -= 4;
            uint16_t ee_adres;
            ee_adres = (uint16_t)(CAN_RX_msg.data_byte[2] << 8);
            ee_adres |= CAN_RX_msg.data_byte[3];
            uint8_t temp[4];
            temp[0] = CAN_RX_msg.data_byte[4];
            temp[1] = CAN_RX_msg.data_byte[5];
            temp[2] = CAN_RX_msg.data_byte[6];
            temp[3] = CAN_RX_msg.data_byte[7];
            eeprom_update_block((const void*) temp, (void*) ee_adres, length);

            /* read */
            length = CAN_RX_msg.length;
            length -= 4;
            ee_adres =  (uint16_t)(CAN_RX_msg.data_byte[2] << 8);
            ee_adres |= CAN_RX_msg.data_byte[3];
            temp[0] = 0;
            temp[1] = 0;
            temp[2] = 0;
            temp[3] = 0;
            eeprom_read_block((void*) temp, (const void*) ee_adres, length);
            CAN_TX_msg.id           = (CAN_Priority_config | module_adres);
            CAN_TX_msg.ext_id       = 0;
            CAN_TX_msg.rtr          = 0;
            CAN_TX_msg.length       = length + 4;
            CAN_TX_msg.data_byte[0] = 0x01;
            CAN_TX_msg.data_byte[1] = 0x03;
            CAN_TX_msg.data_byte[2] = CAN_RX_msg.data_byte[2];
            CAN_TX_msg.data_byte[3] = CAN_RX_msg.data_byte[3];
            CAN_TX_msg.data_byte[4] = temp[0];
            CAN_TX_msg.data_byte[5] = temp[1];
            CAN_TX_msg.data_byte[6] = temp[2];
            CAN_TX_msg.data_byte[7] = temp[3];
            MCP2515_message_TX();

            /*load data in ram */
            if (ee_adres == EE_MICROCONTROLLER_ID)
            {
                microcontroller_id =
                    eeprom_read_word((uint16_t*) EE_MICROCONTROLLER_ID);
            }
            else if (ee_adres == EE_MODULE_ADRES)
            {
                module_adres = eeprom_read_byte((uint8_t*) EE_MODULE_ADRES);
            }
            else if (ee_adres < EE_EXTENDED_DDR0A)
            {
                char* Buffer = "- DO reset -";
                while (*Buffer) { Transmit_USART0(*Buffer++); }
                //                init_io();
            }
            else if (ee_adres > (EE_IO_block - 1))
            {
                build_RAM_IO_from_EEPROM();
            }
            else
            {
                char* Buffer = "- TODO -" DEBUG;
                while (*Buffer) { Transmit_USART0(*Buffer++); }
            }
        }
        else
        {                        /* error in code µc */
            Transmit_USART0(10); /* new line */
            char* Buffer = "- error in code µc -" DEBUG;
            while (*Buffer) { Transmit_USART0(*Buffer++); }
        }
    }

    static void CAN_messag(
        uint8_t module, uint8_t command, uint8_t number, uint8_t toestand)
    {
        uint8_t var = 0;
        for (; var < I_max_block; ++var)
        {
            if (I_from_EEPROM[var][4] < O_max_block)
            {
                if (I_from_EEPROM[var][0] == module)
                {
                    if (I_from_EEPROM[var][1] == command)
                    {
                        if (I_from_EEPROM[var][2] == number)
                        {
                            if (I_from_EEPROM[var][3] == toestand)
                            {
                                if (O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x00)
                                {
                                    /* uitgang uit */
                                    set_port(
                                        O_from_EEPROM[I_from_EEPROM[var][4]][1],
                                        0x00,O_from_EEPROM[I_from_EEPROM[var][4]][2]);
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x01)
                                {
                                    /* uitgang aan */
                                    set_port(
                                        O_from_EEPROM[I_from_EEPROM[var][4]][1],
                                        0x01,O_from_EEPROM[I_from_EEPROM[var][4]][2]);
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x02)
                                {
                                    /* uitgang togel */
                                    set_port(
                                        O_from_EEPROM[I_from_EEPROM[var][4]][1],
                                        0x02,0x00);
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x03)
                                {
                                    /* PWM-uitgang */
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x04)
                                {
                                    /* DAC-uitgang */
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x05)
                                {
                                    /* Timer uitgang uit in sec */
                                    //pin-number = 0x??
                                    //uit        = 0x00
                                    //duur       = 0x01-0xff
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x06)
                                {
                                    /* Timer uitgang aan in sec */
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x07)
                                {
                                    /* Timer uitgang uit in minu */
                                }
                                else if (
                                    O_from_EEPROM[I_from_EEPROM[var][4]][0]
                                    == 0x08)
                                {
                                    /* Timer uitgang aan in minu */
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    static uint8_t input_pol(uint8_t input,uint8_t vpin,uint8_t offset)
    {
        uint8_t set_V_input=vpin;
        uint8_t pin_nr=0x00;

        for(;pin_nr<0x08;++pin_nr)
        {
            if(vpin&(0x01<<pin_nr))
            {
                //was 1
                if(!(input&(0x01<<pin_nr))){
                    //is 0
                    set_V_input&=~(0x01<<pin_nr);

                    //send pin
                    CAN_TX_msg.id           = CAN_Priority_normale | module_adres;
                    CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                    CAN_TX_msg.rtr          = 0;
                    CAN_TX_msg.length       = 3;
                    CAN_TX_msg.data_byte[0] = 0x01; /* 1 ingang */
                    CAN_TX_msg.data_byte[1] = pin_nr+offset;/* ingang nr */
                    CAN_TX_msg.data_byte[2] = 0;/* state */
                    CAN_TX_msg.data_byte[3] = 0;
                    CAN_TX_msg.data_byte[4] = 0;
                    CAN_TX_msg.data_byte[5] = 0;
                    CAN_TX_msg.data_byte[6] = 0;
                    CAN_TX_msg.data_byte[7] = 0;
                    MCP2515_message_TX();
                    CAN_messag((CAN_Priority_normale | module_adres), 0x01, pin_nr+offset, 0);//echo to self
                }
            } else {
                //was 0
                if(input&(0x01<<pin_nr)){
                    //is 1
                    set_V_input|=(0x01<<pin_nr);

                    //send pin
                    CAN_TX_msg.id           = CAN_Priority_normale | module_adres;
                    CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                    CAN_TX_msg.rtr          = 0;
                    CAN_TX_msg.length       = 3;
                    CAN_TX_msg.data_byte[0] = 0x01; /* 1 ingang */
                    CAN_TX_msg.data_byte[1] = pin_nr+offset;/* ingang nr */
                    CAN_TX_msg.data_byte[2] = 1;/* state */
                    CAN_TX_msg.data_byte[3] = 0;
                    CAN_TX_msg.data_byte[4] = 0;
                    CAN_TX_msg.data_byte[5] = 0;
                    CAN_TX_msg.data_byte[6] = 0;
                    CAN_TX_msg.data_byte[7] = 0;
                    MCP2515_message_TX();
                    CAN_messag((CAN_Priority_normale | module_adres), 0x01, pin_nr+offset, 1);//echo to self
                }
            }
        }
        return set_V_input;
    }

    /** main */
    int main(void)
    {
        {
            /* The MCU Status Register provides information on which reset
             * source caused an MCU reset.
             * 0x01 Power-on
             * 0x02 External
             * 0x04 Brown-out
             * 0x05 Brown-out + Power-on
             * 0x06 Brown-out + External
             * 0x07 Brown-out + External + Power-on
             * 0x08 Watchdog
             * 0x10 JTAG
             */
            uint8_t mcusr;
            __asm__ __volatile__ ( "mov %0, r2 \n" : "=r" (mcusr) : );/* MCUSR from bootloader */
            // int8_t Reset_caused_by = MCUSR;
            // MCUSR                  = 0x00; /* Reset the MCUSR */

            /* init */
            microcontroller_id =
                eeprom_read_word((uint16_t*) EE_MICROCONTROLLER_ID);
            module_adres = eeprom_read_byte((uint8_t*) EE_MODULE_ADRES);

            init_io();
            init_USART0();

            /* 0x20 => deze µc is gereset */
            USART_echo_id_Adres(0x20 | mcusr, 0x00);

            /* can */
            MCP2515_init();

            CAN_echo_id_Adres(mcusr, 0x00);
        }

        CAN_TX_msg.id           = 0;
        CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
        CAN_TX_msg.rtr          = 0;
        CAN_TX_msg.length       = 0;
        CAN_TX_msg.data_byte[0] = 0;
        CAN_TX_msg.data_byte[1] = 0;
        CAN_TX_msg.data_byte[2] = 0;
        CAN_TX_msg.data_byte[3] = 0;
        CAN_TX_msg.data_byte[4] = 0;
        CAN_TX_msg.data_byte[5] = 0;
        CAN_TX_msg.data_byte[6] = 0;
        CAN_TX_msg.data_byte[7] = 0;

        build_RAM_IO_from_EEPROM();

        /* adc */
        /* DIDR0 – Digital Input Disable Register 0 */
        DIDR0 = 0xff;

        /* ADC Multiplexer Selection Register */
            /* ADLAR ADC Left Adjust Result */
            ADMUX = (0x01 << ADLAR);
            /*AVCC with external capacitor at AREF pin */
            ADMUX |=(0x01 << REFS0);

        /* ADC Control and Status Register A */
            /* ADPS2:0: ADC Prescaler Select Bits 156.25KHz sample rate @ 20MHz */
            ADCSRA = (0x01 << ADPS2) | (0x01 << ADPS1) | (0x01 << ADPS0);
            /* ADEN: ADC Enable */
            ADCSRA |= (0x01 << ADEN);
            /* Start Conversion */
            ADCSRA |= (0x01 << ADSC);
        uint8_t v_adc_pin[8];

        wdt_enable(WDTO_250MS); /* Watchdog Reset after 250mSec */

        /* Can Watchdog Timer3 */
        TCCR3A=0x00;
        TCCR3B=0x05;/*CSn2 en CSn0*/
        TCCR3C=0x00;
        uint8_t Can_watchdog=0;
        uint8_t poling=0;

        /* input */
        uint8_t v_pinA = (PINA & ~DDRA);
        uint8_t v_pinB = (PINB & ~DDRB);
        uint8_t v_pinC = (PINC & ~DDRC);
        uint8_t v_pinD = (PIND & ~DDRD);
        v_pinA=input_pol(v_pinA,~v_pinA, 0x00);
        v_pinB=input_pol(v_pinB,~v_pinB, 0x08);
        v_pinC=input_pol(v_pinC,~v_pinC, 0x10);
        v_pinD=input_pol(v_pinD,~v_pinD, 0x18);

        /* output zet op de can bus bij opstart */
        uint8_t output=0;
        for (uint8_t pin_nr=0;pin_nr<0x08;++pin_nr,++output) {
            if(0x01&(DDRA<<pin_nr))/* is een output */
                set_port(output,0x01&(PORTA<<pin_nr),0);
        }
        wdt_reset(); /* Reset Watchdog timer*/
        for (uint8_t pin_nr=0;pin_nr<0x08;++pin_nr,++output) {
            if(0x01&(DDRB<<pin_nr))
                set_port(output,0x01&(PORTB<<pin_nr),0);
        }
        wdt_reset(); /* Reset Watchdog timer*/
        for (uint8_t pin_nr=0;pin_nr<0x08;++pin_nr,++output) {
            if(0x01&(DDRC<<pin_nr))
                set_port(output,0x01&(PORTC<<pin_nr),0);
        }
        wdt_reset(); /* Reset Watchdog timer*/
        for (uint8_t pin_nr=0;pin_nr<0x08;++pin_nr,++output) {
            if(0x01&(DDRD<<pin_nr))
                set_port(output,0x01&(PORTD<<pin_nr),0);
        }
        wdt_reset(); /* Reset Watchdog timer*/
        for (;;)
        {
            /* loop */;

            if ((~TCNT3 & 0x1000) && poling==1){
                poling=0;
            }
            if ((TCNT3 & 0x1000) && poling==0){
                poling=1;
                v_pinA=input_pol((PINA&~DDRA), v_pinA, 0x00);
                v_pinB=input_pol((PINB&~DDRB), v_pinB, 0x08);
                v_pinC=input_pol((PINC&~DDRC), v_pinC, 0x10);
                uint8_t masker=(~DDRD);
                if(1){
                    masker&=(~0x04);
                }
                v_pinD=input_pol((PIND&masker), v_pinD, 0x18);

                uint8_t adc_var=ADCH;
                uint8_t adc_pin_nr=(0x07 & ADMUX);
                if(v_adc_pin[adc_pin_nr]<adc_var){
                    if((adc_var-v_adc_pin[adc_pin_nr])>2){//Mod to 2 werkt trager als waarde stijgt

                        v_adc_pin[adc_pin_nr]=adc_var;
                        CAN_TX_msg.id           = (CAN_Priority_normale | module_adres);
                        CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                        CAN_TX_msg.rtr          = 0;
                        CAN_TX_msg.length       = 3;
                        CAN_TX_msg.data_byte[0] = 0x02;
                        CAN_TX_msg.data_byte[1] = adc_pin_nr;
                        CAN_TX_msg.data_byte[2] = adc_var;
                        CAN_TX_msg.data_byte[3] = 0;
                        CAN_TX_msg.data_byte[4] = 0;
                        CAN_TX_msg.data_byte[5] = 0;
                        CAN_TX_msg.data_byte[6] = 0;
                        CAN_TX_msg.data_byte[7] = 0;
                        MCP2515_message_TX();
                    }
                } else if (v_adc_pin[adc_pin_nr]>adc_var) {
                    if((v_adc_pin[adc_pin_nr]-adc_var)>1){

                        v_adc_pin[adc_pin_nr]=adc_var;
                        CAN_TX_msg.id           = (CAN_Priority_normale | module_adres);
                        CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                        CAN_TX_msg.rtr          = 0;
                        CAN_TX_msg.length       = 3;
                        CAN_TX_msg.data_byte[0] = 0x02;
                        CAN_TX_msg.data_byte[1] = adc_pin_nr;
                        CAN_TX_msg.data_byte[2] = adc_var;
                        CAN_TX_msg.data_byte[3] = 0;
                        CAN_TX_msg.data_byte[4] = 0;
                        CAN_TX_msg.data_byte[5] = 0;
                        CAN_TX_msg.data_byte[6] = 0;
                        CAN_TX_msg.data_byte[7] = 0;
                        MCP2515_message_TX();
                    }
                }

                ++adc_pin_nr;
                if(adc_pin_nr>7){adc_pin_nr=0;}
                adc_pin_nr &= 0x07;
                adc_pin_nr |= (ADMUX & 0xe0); //0b11100000
                ADMUX = adc_pin_nr;
                /* Start Conversion */
                ADCSRA |= (0x01 << ADSC);
            }

            if(TCNT3>65000){

                //test uitput
                uint8_t pin_nr=0;
                for (;pin_nr<0xff;++pin_nr) {
                    if(current_O[pin_nr][1]>0){
                        //timer or pwm
                        if(current_O[pin_nr][0]!=0x03)/* niet pwm */{
                            --current_O[pin_nr][1];// verminder de duur
                            if(current_O[pin_nr][1]==0){
                                //mod pin
                                if(current_O[pin_nr][0]==0x00){
                                    //set uitgang aan
                                    set_port(pin_nr,0x01,0x00);
                                } else if (current_O[pin_nr][0]==0x01) {
                                    //set uitgang uit
                                    set_port(pin_nr,0x00,0x00);
                                }
                            } else if (current_O[pin_nr][1]==1) {
                                //zal uit gaan zend can
                                CAN_TX_msg.id           = CAN_Priority_normale | module_adres;
                                CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                                CAN_TX_msg.rtr          = 0;
                                CAN_TX_msg.length       = 4;
                                CAN_TX_msg.data_byte[0] = 0x03; /* 1 uitgang */
                                CAN_TX_msg.data_byte[1] = pin_nr;
                                CAN_TX_msg.data_byte[2] = current_O[pin_nr][0];/* toestand */
                                CAN_TX_msg.data_byte[3] = 0x01;/* duur */
                                CAN_TX_msg.data_byte[4] = 0;
                                CAN_TX_msg.data_byte[5] = 0;
                                CAN_TX_msg.data_byte[6] = 0;
                                CAN_TX_msg.data_byte[7] = 0;
                                MCP2515_message_TX();
                            }
                        }
                    }
                }

                ++Can_watchdog;
                if(Can_watchdog>4){
                    CAN_TX_msg.id           = 0x111;
                    CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                    CAN_TX_msg.rtr          = 1;
                    CAN_TX_msg.length       = 0;
                    CAN_TX_msg.data_byte[0] = 0;
                    CAN_TX_msg.data_byte[1] = 0;
                    CAN_TX_msg.data_byte[2] = 0;
                    CAN_TX_msg.data_byte[3] = 0;
                    CAN_TX_msg.data_byte[4] = 0;
                    CAN_TX_msg.data_byte[5] = 0;
                    CAN_TX_msg.data_byte[6] = 0;
                    CAN_TX_msg.data_byte[7] = 0;
                    MCP2515_message_TX();
                }
                if(Can_watchdog>10){/* 1 ≃~ 3sec*/

                    CAN_TX_msg.id           = (CAN_Priority_config | module_adres);
                    CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                    CAN_TX_msg.rtr          = 0;
                    CAN_TX_msg.length       = 8;
                    CAN_TX_msg.data_byte[0] = 0x00;
                    CAN_TX_msg.data_byte[1] = 0x00;
                    CAN_TX_msg.data_byte[2] = Can_watchdog;
                    CAN_TX_msg.data_byte[3] = MCP2515_check_for_incoming_message();
                    CAN_TX_msg.data_byte[4] = MCP2515_message_RX();
                    CAN_TX_msg.data_byte[5] = 0;
                    CAN_TX_msg.data_byte[6] = 0;
                    CAN_TX_msg.data_byte[7] = 0;
                    MCP2515_message_TX();

                    /* 0x40 => deze µc word gereset */
                    CAN_echo_id_Adres(0x40, 0x40);
                    USART_echo_id_Adres(0x40, 0x40);

                    wdt_enable(WDTO_15MS);/* reset MCU */

                    for (;;){}
                }

                TCNT3=0;
            }
            wdt_reset(); /* Reset Watchdog timer*/
            Receive_USART0();

            /* verwerk de RX USART0 buffer */
            uint8_t RX_BufferCount = RingBuffer_GetCount(&RX_Buffer);
            if (RX_BufferCount > 15) { build_can_block(); }

            if (MCP2515_check_for_incoming_message())
            {
                if (MCP2515_message_RX())
                {
                     /* Reset Watchdog timer can */
                    Can_watchdog=0;

                    /*  global */
                    if (CAN_RX_msg.id == 0x1ff)
                    {
                        CAN_echo_id_Adres(0x00, 0x00);
                    }

                    /*  config */
                    else if (CAN_RX_msg.id == (CAN_Priority_config | module_adres))
                    {
                        if (CAN_RX_msg.data_byte[0] == 0x01) /* EEPROM */
                        {
                            CAN_EEPROM();
                        }
                        else if (CAN_RX_msg.data_byte[0] == 0x02) /* RAM */
                        {
                            Transmit_USART0(10); /* new line */
                            char* Buffer = "- RAM -";
                            while (*Buffer) { Transmit_USART0(*Buffer++); }
                        }
                        else if (CAN_RX_msg.data_byte[0] == 0x03) /* ROM */
                        {
                            Transmit_USART0(10); /* new line */
                            char* Buffer = "- ROM -";
                            while (*Buffer) { Transmit_USART0(*Buffer++); }
                        }
                        else if (CAN_RX_msg.data_byte[0] == 0x04) /* reset */
                        {
                            Transmit_USART0(10); /* new line */
                            char* Buffer = "- reset -";
                            while (*Buffer) { Transmit_USART0(*Buffer++); }
                            if (CAN_RX_msg.data_byte[1] == 0x04)
                            {
                                wdt_enable(WDTO_15MS); /* Watchdog Reset after
                                                          15mSec */

                                /* 0x40 => deze µc word gereset */
                                CAN_echo_id_Adres(0x40, 0x40);
                                USART_echo_id_Adres(0x40, 0x40);

                                for (;;)
                                { /* Watchdog Reset */
                                }
                            }
                        }
                        else
                        {                        /* error in code µc */
                            Transmit_USART0(10); /* new line */
                            char* Buffer = "- error in code µc -" DEBUG;
                            while (*Buffer) { Transmit_USART0(*Buffer++); }
                        }
                    }

                    /* set */
                    else if (CAN_RX_msg.id == (CAN_Priority_set | module_adres))
                    {
                        /* start van protocol
                         * µc   ID 5       01
                         * command  data_byte0 03
                         * number   data_byte1 00-ff
                         * toestand data_byte2 00-ff
                         * data     data_byte3 00-ff
                         */
                        if (CAN_RX_msg.length == 4)
                        {
                            if (CAN_RX_msg.data_byte[0] == 0x03)
                            {
                                set_port(
                                    CAN_RX_msg.data_byte[1],
                                    CAN_RX_msg.data_byte[2],
                                    CAN_RX_msg.data_byte[3]);
                            }
                        }
                    }

                    /* extended */
                    else if (CAN_RX_msg.id == (0x02000000 | microcontroller_id))
                    {
                        if (CAN_RX_msg.length == 3)
                        {
                            if (CAN_RX_msg.data_byte[0] == 0x01)
                            {
                                eeprom_update_byte(
                                    (uint8_t*) EE_MODULE_ADRES,
                                    CAN_RX_msg.data_byte[1]);
                                module_adres = eeprom_read_byte(
                                    (uint8_t*) EE_MODULE_ADRES);
                                if (CAN_RX_msg.data_byte[1] != module_adres)
                                {
                                    module_adres = 0x00; /* reset */
                                }
                            }
                            else if (CAN_RX_msg.data_byte[0] == 0x02)
                            {
                                uint16_t temp =
                                    (uint16_t)(CAN_RX_msg.data_byte[1] << 8);
                                temp |= CAN_RX_msg.data_byte[2];
                                eeprom_update_word(
                                    (uint16_t*) EE_MICROCONTROLLER_ID,
                                    temp);
                                microcontroller_id = eeprom_read_word(
                                    (uint16_t*) EE_MICROCONTROLLER_ID);
                                if (temp != microcontroller_id)
                                {
                                    microcontroller_id = 0x0000; /* reset */
                                }
                            }
                            CAN_echo_id_Adres(0x00, 0x00);
                        }
                    }

                    /* High */
                    /* normale */
                    else if (
                        ((CAN_RX_msg.id & 0xffffff00) == CAN_Priority_High)
                        || ((CAN_RX_msg.id & 0xffffff00) == CAN_Priority_normale))
                    {
                        if(((CAN_RX_msg.id & 0x000000ff) == module_adres) && CAN_RX_msg.rtr==1 )
                        {
                            /* Remote Frame */
                            CAN_TX_msg.id           = CAN_Priority_normale | module_adres;
                            CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                            CAN_TX_msg.rtr          = 0;
                            CAN_TX_msg.length       = 5;
                            CAN_TX_msg.data_byte[0] = 0x05; /* alle ingangen */
                            CAN_TX_msg.data_byte[1] = PINA;
                            CAN_TX_msg.data_byte[2] = PINB;
                            CAN_TX_msg.data_byte[3] = PINC;
                            CAN_TX_msg.data_byte[4] = PIND;
                            CAN_TX_msg.data_byte[5] = 0;
                            CAN_TX_msg.data_byte[6] = 0;
                            CAN_TX_msg.data_byte[7] = 0;
                            MCP2515_message_TX();
                            CAN_TX_msg.id           = CAN_Priority_normale | module_adres;
                            CAN_TX_msg.ext_id       = CAN_STANDARD_FRAME;
                            CAN_TX_msg.rtr          = 0;
                            CAN_TX_msg.length       = 5;
                            CAN_TX_msg.data_byte[0] = 0x06; /* alle uitgangen */
                            CAN_TX_msg.data_byte[1] = PORTA;
                            CAN_TX_msg.data_byte[2] = PORTB;
                            CAN_TX_msg.data_byte[3] = PORTC;
                            CAN_TX_msg.data_byte[4] = PORTD;
                            CAN_TX_msg.data_byte[5] = 0;
                            CAN_TX_msg.data_byte[6] = 0;
                            CAN_TX_msg.data_byte[7] = 0;
                            MCP2515_message_TX();

                        } else {
                            /* test CAN id on list */
                            /* filter module_adres */
                            if (CAN_RX_msg.length > 2)
                            {
                                CAN_messag(
                                    (CAN_RX_msg.id & 0x000000ff),
                                    CAN_RX_msg.data_byte[0],
                                    CAN_RX_msg.data_byte[1],
                                    CAN_RX_msg.data_byte[2]);
                            }
                            else
                            {
                                Transmit_USART0(10); /* new line */
                                char* Buffer = "! CAN length < 3 -" DEBUG;
                                while (*Buffer) { Transmit_USART0(*Buffer++); }
                            }
                        }
                    }

                    typedef union
                    {
                        uint32_t long_id;
                        uint8_t  int_id[8];
                    } ID;
                    ID id;
                    id.long_id = CAN_RX_msg.id;
                    Transmit_USART0(10); /* new line */
                    Transmit_USART0(id.int_id[3]);
                    Transmit_USART0(id.int_id[2]);
                    Transmit_USART0(id.int_id[1]);
                    Transmit_USART0(id.int_id[0]);
                    Transmit_USART0(CAN_RX_msg.ext_id);
                    Transmit_USART0(CAN_RX_msg.rtr);
                    Transmit_USART0(CAN_RX_msg.length);
                    uint8_t var = 0;
                    for (; var < 8; ++var)
                    {
                        if (var < CAN_RX_msg.length)
                        {
                            Transmit_USART0(CAN_RX_msg.data_byte[var]);
                        }
                        else
                        {
                            Transmit_USART0(0x00);
                        }
                    }
                }
            }
        }
        return 0;
    }

#ifdef __cplusplus
} // extern "C"
#endif

#endif
