#ifndef CAN_MCP2515_H
#define CAN_MCP2515_H

#include "../include/SPI.h"

#include <avr/io.h>

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef SPI_CS_PIN
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
#    define SPI_CS_PIN = 9;
#endif

/** \brief Settings of the CS pin
 *
 * This function is used for setting of the CS pin. CS signal
 * is inverted, so input 1 (1) means zero on the output.
 * Otherwise is analogically the same.
 *
 * \warning This is platform-dependent method!
 * \param state Wished state
 */
#define MCP2515_SELECT() SPI_PORT_CAN &= ~(0x01 << SPI_CS_CAN); /* laag */

/** \brief Settings of the CS pin
 *
 * This function is used for setting of the CS pin. CS signal
 * is inverted, so input 1 (1) means zero on the output.
 * Otherwise is analogically the same.
 *
 * \warning This is platform-dependent method!
 * \param state Wished state
 */
#define MCP2515_UNSELECT() SPI_PORT_CAN |= (0x01 << SPI_CS_CAN); /* high */

    /*              Commands            */

#define SPI_RESET 0xC0 /* Resets internal registers to the default state */
#define SPI_READ                                                               \
    0x03 /* Reads data from the register beginning at selected address. */
#define SPI_READ_RX                                                            \
    0x90 /* When reading a receive buffer, reduces the overhead of a normal    \
            READ command by placing the Address Pointer at one of four         \
            locations, as indicated by ‘n,m’. */
#define SPI_WRITE                                                              \
    0x02 /* Writes data to the register beginning at the selected address. */
#define SPI_WRITE_TX                                                           \
    0x40 /* When loading a transmit buffer, reduces the overhead of a normal   \
            WRITE command by placing the Address Pointer at one of six         \
            locations, as indicated by ‘a,b,c’. */
#define SPI_RTS                                                                \
    0x80 /* Instructs controller to begin message transmission sequence for    \
            any of the transmit buffers */
#define SPI_READ_STATUS                                                        \
    0xA0 /* Quick polling command that reads several status bits for transmit  \
            and receive functions. */
#define SPI_RX_STATUS                                                          \
    0xB0 /* Quick polling command that indicates filter match and message type \
            (standard, extended and/or remote) of received message. */
#define SPI_BIT_MODIFY                                                         \
    0x05 /* Allows the user to set or clear individual bits in a particular    \
            register. */

    /*        Register Addresses        */

#define RXF0SIDH  0x00
#define RXF0SIDL  0x01
#define RXF0EID8  0x02
#define RXF0EID0  0x03
#define RXF1SIDH  0x04
#define RXF1SIDL  0x05
#define RXF1EID8  0x06
#define RXF1EID0  0x07
#define RXF2SIDH  0x08
#define RXF2SIDL  0x09
#define RXF2EID8  0x0A
#define RXF2EID0  0x0B
#define BFPCTRL   0x0C /* RXnBF PIN CONTROL AND STATUS REGISTER */
#define TXRTSCTRL 0x0D /* TXnRTS PIN CONTROL AND STATUS REGISTER */
#define CANSTAT   0x0E /* CAN STATUS REGISTER */
#define CANCTRL   0x0F /* CAN CONTROL REGISTER */

#define RXF3SIDH 0x10
#define RXF3SIDL 0x11
#define RXF3EID8 0x12
#define RXF3EID0 0x13
#define RXF4SIDH 0x14
#define RXF4SIDL 0x15
#define RXF4EID8 0x16
#define RXF4EID0 0x17
#define RXF5SIDH 0x18
#define RXF5SIDL 0x19
#define RXF5EID8 0x1A
#define RXF5EID0 0x1B
#define TEC      0x1C
#define REC      0x1D

#define RXM0SIDH 0x20 /*  */
#define RXM0SIDL 0x21 /*  */
#define RXM0EID8 0x22 /*  */
#define RXM0EID0 0x23 /*  */
#define RXM1SIDH 0x24 /*  */
#define RXM1SIDL 0x25 /*  */
#define RXM1EID8 0x26 /*  */
#define RXM1EID0 0x27 /*  */
#define CNF3     0x28 /* CONFIGURATION REGISTER 3 */
#define CNF2     0x29 /* CONFIGURATION REGISTER 2 */
#define CNF1     0x2A /* CONFIGURATION REGISTER 1 */
#define CANINTE  0x2B /* CAN INTERRUPT ENABLE REGISTER */
#define CANINTF  0x2C /* CAN INTERRUPT FLAG REGISTER */
#define EFLG     0x2D /* ERROR FLAG REGISTER */

#define TXB0CTRL 0x30 /* TRANSMIT BUFFER n CONTROL REGISTER */
#define TXB0SIDH 0x31 /* TRANSMIT BUFFER n STANDARD IDENTIFIER REGISTER HIGH   \
                       */
#define TXB0SIDL 0x32 /* TRANSMIT BUFFER n STANDARD IDENTIFIER REGISTER LOW */
#define TXB0EID8                                                               \
    0x33 /* TRANSMIT BUFFER n EXTENDED IDENTIFIER 8 REGISTER HIGH */
#define TXB0EID0 0x34 /* TRANSMIT BUFFER n EXTENDED IDENTIFIER 0 REGISTER LOW  \
                       */
#define TXB0DLC 0x35  /* TRANSMIT BUFFER n DATA LENGTH CODE REGISTER */
#define TXB0D0  0x36  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB0D1  0x37  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB0D2  0x38  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB0D3  0x39  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB0D4  0x3A  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB0D5  0x3B  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB0D6  0x3C  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB0D7  0x3D  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */

#define TXB1CTRL 0x40 /* TRANSMIT BUFFER n CONTROL REGISTER */
#define TXB1SIDH 0x41 /* TRANSMIT BUFFER n STANDARD IDENTIFIER REGISTER HIGH   \
                       */
#define TXB1SIDL 0x42 /* TRANSMIT BUFFER n STANDARD IDENTIFIER REGISTER LOW */
#define TXB1EID8                                                               \
    0x43 /* TRANSMIT BUFFER n EXTENDED IDENTIFIER 8 REGISTER HIGH */
#define TXB1EID0 0x44 /* TRANSMIT BUFFER n EXTENDED IDENTIFIER 0 REGISTER LOW  \
                       */
#define TXB1DLC 0x45  /* TRANSMIT BUFFER n DATA LENGTH CODE REGISTER */
#define TXB1D0  0x46  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB1D1  0x47  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB1D2  0x48  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB1D3  0x49  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB1D4  0x4A  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB1D5  0x4B  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB1D6  0x4C  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB1D7  0x4D  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */

#define TXB2CTRL 0x50 /* TRANSMIT BUFFER n CONTROL REGISTER */
#define TXB2SIDH 0x51 /* TRANSMIT BUFFER n STANDARD IDENTIFIER REGISTER HIGH   \
                       */
#define TXB2SIDL 0x52 /* TRANSMIT BUFFER n STANDARD IDENTIFIER REGISTER LOW */
#define TXB2EID8                                                               \
    0x53 /* TRANSMIT BUFFER n EXTENDED IDENTIFIER 8 REGISTER HIGH */
#define TXB2EID0 0x54 /* TRANSMIT BUFFER n EXTENDED IDENTIFIER 0 REGISTER LOW  \
                       */
#define TXB2DLC 0x55  /* TRANSMIT BUFFER n DATA LENGTH CODE REGISTER */
#define TXB2D0  0x56  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB2D1  0x57  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB2D2  0x58  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB2D3  0x59  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB2D4  0x5A  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB2D5  0x5B  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB2D6  0x5C  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */
#define TXB2D7  0x5D  /* TRANSMIT BUFFER n DATA BYTE m REGISTER */

#define RXB0CTRL 0x60 /* RECEIVE BUFFER 0 CONTROL REGISTER */
#define RXB0SIDH 0x61 /* RECEIVE BUFFER n STANDARD IDENTIFIER REGISTER HIGH */
#define RXB0SIDL 0x62 /* RECEIVE BUFFER n STANDARD IDENTIFIER REGISTER LOW */
#define RXB0EID8 0x63 /* RECEIVE BUFFER n EXTENDED IDENTIFIER REGISTER HIGH */
#define RXB0EID0 0x64 /* RECEIVE BUFFER n EXTENDED IDENTIFIER REGISTER LOW */
#define RXB0DLC  0x65 /* RECEIVE BUFFER n DATA LENGTH CODE REGISTER */
#define RXB0D0   0x66 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB0D1   0x67 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB0D2   0x68 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB0D3   0x69 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB0D4   0x6A /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB0D5   0x6B /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB0D6   0x6C /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB0D7   0x6D /* RECEIVE BUFFER n DATA BYTE m REGISTER */

#define RXB1CTRL 0x70 /* RECEIVE BUFFER 1 CONTROL REGISTER */
#define RXB1SIDH 0x71 /* RECEIVE BUFFER n STANDARD IDENTIFIER REGISTER HIGH */
#define RXB1SIDL 0x72 /* RECEIVE BUFFER n STANDARD IDENTIFIER REGISTER LOW */
#define RXB1EID8 0x73 /* RECEIVE BUFFER n EXTENDED IDENTIFIER REGISTER HIGH */
#define RXB1EID0 0x74 /* RECEIVE BUFFER n EXTENDED IDENTIFIER REGISTER LOW */
#define RXB1DLC  0x75 /* RECEIVE BUFFER n DATA LENGTH CODE REGISTER */
#define RXB1D0   0x76 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB1D1   0x77 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB1D2   0x78 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB1D3   0x79 /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB1D4   0x7A /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB1D5   0x7B /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB1D6   0x7C /* RECEIVE BUFFER n DATA BYTE m REGISTER */
#define RXB1D7   0x7D /* RECEIVE BUFFER n DATA BYTE m REGISTER */

/*            Bit Definition of BFPCTRL       */
#define B1BFS 5
#define B0BFS 4
#define B1BFE 3
#define B0BFE 2
#define B1BFM 1
#define B0BFM 0

/*      Bit Definition of TXRTSCTRL       */
#define B2RTS  5
#define B1RTS  4
#define B0RTS  3
#define B2RTSM 2
#define B1RTSM 1
#define B0RTSM 0

/*            Bit Definition of CANSTAT     */
#define OPMOD2 7
#define OPMOD1 6
#define OPMOD0 5
#define ICOD2  3
#define ICOD1  2
#define ICOD0  1

/*            Bit Definition of CANCTRL     */
#define REQOP2  7
#define REQOP1  6
#define REQOP0  5
#define ABAT    4
#define CLKEN   2
#define CLKPRE1 1
#define CLKPRE0 0

/*            Bit Definition of CNF3        */
#define WAKFIL  6
#define PHSEG22 2
#define PHSEG21 1
#define PHSEG20 0

/*            Bit Definition of CNF2        */
#define BTLMODE 7
#define SAM     6
#define PHSEG12 5
#define PHSEG11 4
#define PHSEG10 3
#define PHSEG2  2
#define PHSEG1  1
#define PHSEG0  0

/*            Bit Definition of CNF1        */
#define SJW1 7
#define SJW0 6
#define BRP5 5
#define BRP4 4
#define BRP3 3
#define BRP2 2
#define BRP1 1
#define BRP0 0

/*            Bit Definition of CANINTE     */
#define MERRE 7
#define WAKIE 6
#define ERRIE 5
#define TX2IE 4
#define TX1IE 3
#define TX0IE 2
#define RX1IE 1
#define RX0IE 0

/*            Bit Definition of CANINTF     */
#define MERRF 7
#define WAKIF 6
#define ERRIF 5
#define TX2IF 4
#define TX1IF 3
#define TX0IF 2
#define RX1IF 1
#define RX0IF 0

/*            Bit Definition of EFLG        */
#define RX1OVR 7
#define RX0OVR 6
#define TXB0   5
#define TXEP   4
#define RXEP   3
#define TXWAR  2
#define RXWAR  1
#define EWARN  0

/*            Bit Definition of TXB0CTRL    */
/*            Bit Definition of TXB1CTRL    */
/*            Bit Definition of TXB2CTRL    */
#define ABTF  6
#define MLOA  5
#define TXERR 4
#define TXREQ 3
#define TXP1  1
#define TXP0  0

/*            Bit Definition of RXB0CTRL    */
#define RXM1    6
#define RXM0    5
#define RXRTR   3
#define BUKT    2
#define BUKT1   1
#define FILHIT0 0

/*            Bit Definition of TXB0SIDL    */
/*            Bit Definition of TXB1SIDL    */
#define EXIDE 3

/*            Bit Definition of RXB1CTRL    */
#define FILHIT2 2
#define FILHIT1 1

/*            Bit Definition of RXB0SIDL    */
/*            Bit Definition of RXB1SIDL    */
#define SRR 4
#define IDE 3

    /*            Bit Definition of RXB0DLC     */
    /*            Bit Definition of RXB1DLC     */

#define RTR  6
#define DLC3 3
#define DLC2 2
#define DLC1 1
#define DLC0 0

#define CAN_STANDARD_FRAME 0
#define CAN_EXTENDED_FRAME 1

#define CAN_STANDARD_ID_MASK 0x07FF

#define CAN_EXTENDED_ID_MASK 0x1FFFFFFF

#define CAN_DATA_FIELD_LENGTH 8

#define SIDL_SID          0xE0
#define SIDL_EID          0x03
#define length_bit_fields 0x0F

// CAN Bitrate 125 kbps
#define R_CNF1_125kbps ((1 << BRP2) | (1 << BRP1) | (1 << BRP0))
#define R_CNF2_125kbps ((1 << BTLMODE) | (1 << PHSEG11))
#define R_CNF3_125kbps ((1 << PHSEG21))

// CAN Bitrate 250 kbps
#define R_CNF1_250kbps ((1 << BRP1) | (1 << BRP0))
#define R_CNF2_250kbps ((1 << BTLMODE) | (1 << PHSEG11))
#define R_CNF3_250kbps ((1 << PHSEG21))

// CAN Bitrate 500 kbps
#define R_CNF1_500kbps ((1 << BRP0))
#define R_CNF2_500kbps ((1 << BTLMODE) | (1 << PHSEG11))
#define R_CNF3_500kbps ((1 << PHSEG21))

// CAN Bitrate 1 Mbps
#define R_CNF1_1Mbps 0
#define R_CNF2_1Mbps ((1 << BTLMODE) | (1 << PHSEG11))
#define R_CNF3_1Mbps ((1 << PHSEG21))

    typedef struct CAN_msg
    {
        unsigned long id;
        unsigned char ext_id : 1;
        unsigned char rtr : 1;
        unsigned char length : 4;
        unsigned char data_byte[CAN_DATA_FIELD_LENGTH];
    };

    struct CAN_msg CAN_TX_msg;
    struct CAN_msg CAN_RX_msg;

    void MCP2515_init(void);
    void MCP2515_reset(void);
    void MCP2515_write_register(unsigned char address, unsigned char value);
    unsigned char MCP2515_read_register(unsigned char address);
    void          MCP2515_bit_modify(
                 unsigned char address, unsigned char bitmask, unsigned char value);
    unsigned char MCP2515_read_status(unsigned char type);
    unsigned char MSP2515_check_free_buffer(void);
    unsigned char MCP2515_check_for_incoming_message(void);
    void          MCP2515_check_for_interrupts(void);
    unsigned char MCP2515_message_TX(void);
    unsigned char MCP2515_message_RX(void);
    void          MCP2515_clear_RX_buffers(void);
    void          MCP2515_clear_TX_buffers(void);
    void          MCP2515_flush_buffers(void);
    void          MCP2515_set_filter(
                 unsigned char filter_number,
                 unsigned long filter_id,
                 unsigned char ext_std_frame);
    void MCP2515_set_mask(
        unsigned char mask_number,
        unsigned long mask_id,
        unsigned char ext_std_frame);
#ifdef __cplusplus
} // extern "C"
#endif
#endif // CAN_MCP2515_H
