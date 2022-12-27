#include "../include/CAN_MCP2515.h"

#include <util/delay.h>

void MCP2515_init(void)
{
    init_SPI();
    SPI_DDR_CAN |= (0x01 << SPI_CS_CAN);   /* output */
    SPI_PORT_CAN &= ~(0x01 << SPI_CS_CAN); /* laag */

    INT_DDR_CAN &= ~(0x01 << INT_CAN);  /* input */
    INT_PORT_CAN |= (0x01 << INT_CAN); /* pul-up */

    //    _delay_ms(10);

    MCP2515_reset();
    //    _delay_us(10);
    uint8_t teller = 0;
    while (0x80 != (MCP2515_read_register(CANSTAT) & 0xE0))
    { /* is in Configuration Mode? see 10.0 */
        if (teller > 200)
        {
            MCP2515_reset();
            teller = 0;
        }
        teller++;
    }
    /* Configuration Mode 10.1 */

    /* CNF1 CNF2 CNF3 register */
    //    MCP2515_write_register(CNF3, R_CNF3_250kbps);//2
    //    MCP2515_write_register(CNF2, R_CNF2_250kbps);//90
    //    MCP2515_write_register(CNF1, R_CNF1_250kbps);//03

    /*
     *    Tq
SyncSeg   1
PropSeg   3
PhaseSeg1 2
PhaseSeg2 2
8*250ns =2µs
    */
    /* CLKOUT pin is enabled for clock out function
     * Wake-up filter is disabled
     * PS2 Length bits:
     * (PHSEG2[2:0] + 1) x TQ. Minimum valid setting for PS2 is 2 TQs. */
    MCP2515_write_register(CNF3, 0x01);

    /* Length of PS2 is determined by the PHSEG2[2:0] bits of CNF3
     * Bus line is sampled once at the sample point
     * PS1 Length bits (1 + 1) x TQ.
     * Propagation Segment Length bits (2 + 1) x TQ. */
    MCP2515_write_register(CNF2, 0x8A);

    /*  Synchronization Jump Width: Length = 1 x TQ
     *  Baud Rate Prescaler bits  :TQ = 2 x (1 + 1)/FOSC(8Mhz) = 250ns */
    MCP2515_write_register(CNF1, 0x01);

    /* set interrupt when message was received, Message Error Interrupt */
    //    MCP2515_write_register(
    //        CANINTE,
    //        ((1 << RX1IE) | (1 << RX0IE) | (1 << MERRE)));

    /* set interrupt when message was received */
    MCP2515_write_register(CANINTE, ((1 << RX1IE) | (1 << RX0IE)));

    /* Turns mask/filters off; receives any message */
    MCP2515_write_register(RXB0CTRL, ((1 << RXM1) | (1 << RXM0)));
    MCP2515_write_register(RXB1CTRL, ((1 << RXM1) | (1 << RXM0)));

    /* RX0BF and RX1BF Pin function is disabled,
     * pin goes to a high-impedance state */
    MCP2515_write_register(BFPCTRL, 0);
    /* TX0RTS TX1RTS TX2RTS Pin mode bit
     * = Digital input 100komh internal pull-up to VDD*/
    MCP2515_write_register(TXRTSCTRL, 0);

    MCP2515_set_mask(0, 0x00000000, CAN_STANDARD_FRAME);
    MCP2515_set_mask(1, 0x00000000, CAN_STANDARD_FRAME);

    MCP2515_set_filter(0, 0x00000000, CAN_STANDARD_FRAME);
    MCP2515_set_filter(1, 0x00000000, CAN_STANDARD_FRAME);
    MCP2515_set_filter(2, 0x00000000, CAN_STANDARD_FRAME);
    MCP2515_set_filter(3, 0x00000000, CAN_STANDARD_FRAME);
    MCP2515_set_filter(4, 0x00000000, CAN_STANDARD_FRAME);
    MCP2515_set_filter(5, 0x00000000, CAN_STANDARD_FRAME);

    MCP2515_flush_buffers();

    /* Sets Normal Operation mode */
    MCP2515_bit_modify(CANCTRL, 0xE0, 0x00);
}

/**
 * @brief MCP2515_reset
 * 12.2 RESET Instruction
 *
 * The RESET instruction can be used to reinitialize the internal registers of
 * the MCP2515 and set the Configuration mode. This command provides the same
 * functionality, via the SPI interface, as the RESET pin.
 *
 * The RESET instruction is a single byte instruction that requires selecting
 * the device by pulling the CS pin low, sending the instruction byte and then
 * raising the CS pin. It is highly recommended that the RESET command be sent
 * (or the RESET pin be lowered) as part of the power-on initialization
 * sequence.
 **/
void MCP2515_reset(void)
{
    MCP2515_SELECT();
    spi_write(SPI_RESET);
    MCP2515_UNSELECT();
}

/**
 * @brief SPI: 0x02 Writes data to the register
 *
 * Function name:           MCP2515_write_register
 * Descriptions:            set register
 *
 * Change value of the register on selected address inside the
 * MCP2515. Works for every register.
 *
 * \see MCP2515 datasheet, chapter 11 - register description
 * \see MCP2515 datasheet, chapter 12 - write instruction
 * \param address Register address
 * \param value New value of the register
 **/
void MCP2515_write_register(unsigned char address, unsigned char value)
{
    MCP2515_SELECT();
    spi_write(SPI_WRITE);
    spi_write(address);
    spi_write(value);
    MCP2515_UNSELECT();
}

/**
 *@brief SPI: 0x03 Reads data from the register
 *
 * Read value of the register on selected address inside the
 * MCP2515. Works for every register.
 *
 * \see MCP2515 datasheet, chapter 11 - register description
 * \see MCP2515 datasheet, chapter 12 - read instruction
 * \param address Register address
 **/
unsigned char MCP2515_read_register(unsigned char address)
{
    /* Send read instruction, address, and receive result */
    unsigned char value = 0x00;

    MCP2515_SELECT();
    spi_write(SPI_READ);
    spi_write(address);
    value = spi_read();
    MCP2515_UNSELECT();

    return value;
}

/**
 * @brief SPI: 0x05 BIT MODIFY INSTRUCTION
 * Function name:           MCP2515_bit_modify
 * Descriptions:            set bit of one register
 **/
void MCP2515_bit_modify(
    unsigned char address, unsigned char bitmask, unsigned char value)
{
    MCP2515_SELECT();
    spi_write(SPI_BIT_MODIFY);
    spi_write(address);
    spi_write(bitmask);
    spi_write(value);
    MCP2515_UNSELECT();
}

unsigned char MCP2515_read_status(unsigned char type)
{
    unsigned char value = 0x00;

    MCP2515_SELECT();
    spi_write(type);
    value = spi_read();
    MCP2515_UNSELECT();

    return value;
}

unsigned char MSP2515_check_free_buffer(void)
{
    unsigned char status = 0x00;

    status = MCP2515_read_status(SPI_READ_STATUS);
    if ((status & 0x54) == 0x54) { return 0; }

    return 1;
}

unsigned char MCP2515_check_for_incoming_message(void)
{
    return (! (INT_PIN_CAN & (1 << INT_CAN)));
}

/**
 * @brief MCP2515_check_for_interrupts
 */
void MCP2515_check_for_interrupts(void)
{
    uint8_t can_interrupt = MCP2515_read_register(CANINTF);
    if (can_interrupt != 0)
    {
        /* process interrupt */
        if (can_interrupt & (1 << RX0IF))
        {
            /* Receive Buffer 1 Full Interrupt */
            /* ToDo process */

            MCP2515_message_RX();

            MCP2515_bit_modify(CANINTF, (1 << RX0IF), 0x00);
        }
        if (can_interrupt & (1 << RX1IF))
        {
            /* Receive Buffer 2 Full Interrupt */
            /* ToDo process */
            MCP2515_message_RX();

            MCP2515_bit_modify(CANINTF, (1 << RX1IF), 0x00);
        }
        if (can_interrupt & (1 << TX0IF))
        {
            /* Transmit Buffer 0 Empty Interrupt */
            /* ToDo process */
            MCP2515_bit_modify(CANINTF, (1 << TX0IF), 0x00);
        }
        if (can_interrupt & (1 << TX1IF))
        {
            /* Transmit Buffer 1 Empty Interrupt */
            /* ToDo process */
            MCP2515_bit_modify(CANINTF, (1 << TX1IF), 0x00);
        }
        if (can_interrupt & (1 << TX2IF))
        {
            /* Transmit Buffer 2 Empty Interrupt */
            /* ToDo process */
            MCP2515_bit_modify(CANINTF, (1 << TX2IF), 0x00);
        }
        if (can_interrupt & (1 << ERRIF))
        {
            /* Error Interrupt */
            MCP2515_read_register(EFLG);
            MCP2515_read_register(TEC);
            MCP2515_read_register(REC);
            /* ToDo process */
            MCP2515_bit_modify(CANINTF, (1 << ERRIF), 0x00);
        }
        if (can_interrupt & (1 << WAKIF))
        {
            /* Wake-up Interrupt */
            /* ToDo process */
            MCP2515_bit_modify(CANINTF, (1 << WAKIF), 0x00);
        }
        if (can_interrupt & (1 << MERRF))
        {
            /* Message Error Interrupt */
            MCP2515_read_register(EFLG);
            MCP2515_read_register(TEC);
            MCP2515_read_register(REC);
            /* ToDo process */
            MCP2515_bit_modify(CANINTF, (1 << MERRF), 0x00);
        }
    }

    // test soms geen can com meer
    MCP2515_reset();
}
/**
 * @brief MCP2515_message_TX
 * @return
 * 0=Error no Transmit buffer empty
 * 1=ok message to Transmit buffer TXB0
 * 2=ok message to Transmit buffer TXB1
 * 4=ok message to Transmit buffer TXB2
 */
unsigned char MCP2515_message_TX(void)
{
    unsigned char i        = 0x00;
    unsigned char status   = 0x00;
    unsigned char address  = 0x00;
    unsigned char TXBnDLC  = 0x00;
    unsigned char TXBnSIDH = 0x00;
    unsigned char TXBnSIDL = 0x00;
    unsigned char TXBnEID8 = 0x00;
    unsigned char TXBnEID0 = 0x00;

    TXBnDLC = (CAN_TX_msg.length & length_bit_fields);

    /* which Transmit buffers is empty? */
    status = MCP2515_read_status(SPI_READ_STATUS);

    if ((status & (1 << 2)) == 0)
    {
        /* Transmit buffer empty TXREQ TXB0CNTRL */
        /* address = 0x00; */
    }
    else if ((status & (1 << 4)) == 0)
    {
        /* Transmit buffer empty TXREQ TXB1CNTRL */
        address = 0x02;
    }
    else if ((status & (1 << 6)) == 0)
    {
        /* Transmit buffer empty TXREQ TXB2CNTRL */
        address = 0x04;
    }
    else
    {
        /* Error no Transmit buffer empty */
        return 0;
    }

    switch (CAN_TX_msg.ext_id)
    {
    case CAN_EXTENDED_FRAME:
    {
        TXBnSIDH = (CAN_TX_msg.id >> 21);
        TXBnSIDL = ((CAN_TX_msg.id >> 13) & SIDL_SID);
        TXBnSIDL |= ((CAN_TX_msg.id >> 16) & SIDL_EID);
        TXBnEID8 = (CAN_TX_msg.id >> 8);
        TXBnEID0 = CAN_TX_msg.id;

        TXBnSIDL |= (1 << IDE);

        if (CAN_TX_msg.rtr) { TXBnDLC |= (1 << RTR); }

        break;
    }

    default:
    {
        TXBnSIDH = (CAN_TX_msg.id >> 3);
        TXBnSIDL = ((CAN_TX_msg.id << 5) & SIDL_SID);
        TXBnEID8 = 0x00;
        TXBnEID0 = 0x00;

        if (CAN_TX_msg.rtr) { TXBnDLC |= (1 << RTR); }

        break;
    }
    }

    MCP2515_SELECT();
    spi_write(SPI_WRITE_TX | address);

    spi_write(TXBnSIDH);
    spi_write(TXBnSIDL);
    spi_write(TXBnEID8);
    spi_write(TXBnEID0);
    spi_write(TXBnDLC);

    if (!CAN_TX_msg.rtr){for (i = 0; i < TXBnDLC; i++) { spi_write(CAN_TX_msg.data_byte[i]); }}

    MCP2515_UNSELECT();

    /* CS Setup Time min 50ns */

    if (address == 0) { address = 1; }

    MCP2515_SELECT();

    spi_write(SPI_RTS | address);
    MCP2515_UNSELECT();

    return address;
}

unsigned char MCP2515_message_RX(void)
{
    unsigned char i        = 0x00;
    unsigned char status   = 0x00;
    unsigned char address  = 0x00;
    unsigned char RXBnDLC  = 0x00;
    unsigned char RXBnSIDH = 0x00;
    unsigned char RXBnSIDL = 0x00;
    unsigned char RXBnEID8 = 0x00;
    unsigned char RXBnEID0 = 0x00;

    unsigned long temp = 0;

    status = MCP2515_read_status(SPI_RX_STATUS);

    if ((status & (1 << 6)) != 0) { address = SPI_READ_RX; }
    else if ((status & (1 << 7)) != 0)
    {
        address = (SPI_READ_RX | 0x04);
    }
    else
    {
        MCP2515_check_for_interrupts();
        return 0; // 18 0001 1000
    }

    MCP2515_SELECT();
    spi_write(address);

    RXBnSIDH = spi_read();
    RXBnSIDL = spi_read();
    RXBnEID8 = spi_read();
    RXBnEID0 = spi_read();
    RXBnDLC  = spi_read();

    CAN_RX_msg.length = (RXBnDLC & length_bit_fields);

    for (i = 0; i < CAN_RX_msg.length; i++)
    { CAN_RX_msg.data_byte[i] = spi_read(); }

    MCP2515_UNSELECT();

    CAN_RX_msg.ext_id = (RXBnSIDL & 0x08);

    if ((RXBnSIDL & 0x08)) /* CAN_EXTENDED_FRAME */
    {
        temp = ((unsigned long) RXBnSIDH);
        temp <<= 21;
        CAN_RX_msg.id = temp;

        temp = ((unsigned long) (RXBnSIDL & SIDL_SID));
        temp <<= 13;
        CAN_RX_msg.id |= temp;

        temp = ((unsigned long) (RXBnSIDL & SIDL_EID));
        temp <<= 16;
        CAN_RX_msg.id |= temp;

        temp = ((unsigned long) RXBnEID8);
        temp <<= 8;
        CAN_RX_msg.id |= temp;

        temp = ((unsigned long) RXBnEID0);
        CAN_RX_msg.id |= temp;

        CAN_RX_msg.rtr = (RXBnDLC & (1 << RTR));
    }
    else
    {
        temp          = ((unsigned long) RXBnSIDH);
        CAN_RX_msg.id = (temp << 3);

        temp = ((unsigned long) RXBnSIDL);
        temp &= SIDL_SID;
        CAN_RX_msg.id |= (temp >> 5);

        CAN_RX_msg.rtr = (RXBnSIDL & (1 << SRR));
    }

    if ((status & (1 << 6)) != 0)
    { MCP2515_bit_modify(CANINTF, (1 << RX0IF), 0); }
    else
    {
        MCP2515_bit_modify(CANINTF, (1 << RX1IF), 0);
    }
    return (1 + (status & 0x07));
}

void MCP2515_clear_RX_buffers(void)
{
    unsigned char i = 0;

    MCP2515_SELECT();
    spi_write(SPI_WRITE);
    spi_write(RXB0SIDH);

    for (i = 0; i < 13; i++) { spi_write(0x00); }

    MCP2515_UNSELECT();
    /* CS Hold Time= Min 50ns is nu 100ns met µc 20Mhz */
    MCP2515_SELECT();
    spi_write(SPI_WRITE);
    spi_write(RXB1SIDH);

    for (i = 0; i < 13; i++) { spi_write(0x00); }

    MCP2515_UNSELECT();
}

void MCP2515_clear_TX_buffers(void)
{
    unsigned char i = 0;

    MCP2515_SELECT();
    spi_write(SPI_WRITE);
    spi_write(TXB0SIDH);

    for (i = 0; i < 13; i++) { spi_write(0x00); }

    MCP2515_UNSELECT();

    MCP2515_SELECT();
    spi_write(SPI_WRITE);
    spi_write(TXB1SIDH);

    for (i = 0; i < 13; i++) { spi_write(0x00); }

    MCP2515_UNSELECT();

    MCP2515_SELECT();
    spi_write(SPI_WRITE);
    spi_write(TXB2SIDH);

    for (i = 0; i < 13; i++) { spi_write(0x00); }

    MCP2515_UNSELECT();
}

/**
 * @brief MCP2515_flush_buffers clear RX & TX buffers
 */
void MCP2515_flush_buffers(void)
{
    MCP2515_clear_RX_buffers();
    MCP2515_clear_TX_buffers();
}

void MCP2515_set_filter(
    unsigned char filter_number,
    unsigned long filter_id,
    unsigned char ext_std_frame)
{
    unsigned char address  = 0x00;
    unsigned char RXFnSIDH = 0x00;
    unsigned char RXFnSIDL = 0x00;
    unsigned char RXFnEID8 = 0x00;
    unsigned char RXFnEID0 = 0x00;

    switch (filter_number)
    {
    case 1:
    {
        address = RXF1SIDH;
        break;
    }

    case 2:
    {
        address = RXF2SIDH;
        break;
    }

    case 3:
    {
        address = RXF3SIDH;
        break;
    }

    case 4:
    {
        address = RXF4SIDH;
        break;
    }

    case 5:
    {
        address = RXF5SIDH;
        break;
    }

    default:
    {
        address = RXF0SIDH;
        break;
    }
    }

    switch (ext_std_frame)
    {
    case CAN_EXTENDED_FRAME:
    {
        RXFnSIDH = (filter_id >> 21);
        RXFnSIDL = ((filter_id >> 13) & SIDL_SID);
        RXFnSIDL |= ((filter_id >> 16) & SIDL_EID);
        RXFnEID8 = (filter_id >> 8);
        RXFnEID0 = filter_id;

        RXFnSIDL |= (1 << EXIDE);

        break;
    }

    default:
    {
        RXFnSIDH = (filter_id >> 3);
        RXFnSIDL = ((filter_id << 5) & SIDL_SID);
        RXFnEID8 = 0x00;
        RXFnEID0 = 0x00;

        break;
    }
    }

    MCP2515_SELECT();
    spi_write(address);
    spi_write(RXFnSIDH);
    spi_write(RXFnSIDL);
    spi_write(RXFnEID8);
    spi_write(RXFnEID0);
    MCP2515_UNSELECT();
}

void MCP2515_set_mask(
    unsigned char mask_number,
    unsigned long mask_id,
    unsigned char ext_std_frame)
{
    unsigned char address  = 0x00;
    unsigned char RXMnSIDH = 0x00;
    unsigned char RXMnSIDL = 0x00;
    unsigned char RXMnEID8 = 0x00;
    unsigned char RXMnEID0 = 0x00;

    switch (mask_number)
    {
    case 1:
    {
        address = RXM0SIDH;
        break;
    }

    default:
    {
        address = RXM1SIDH;
        break;
    }
    }

    switch (ext_std_frame)
    {
    case CAN_EXTENDED_FRAME:
    {
        RXMnSIDH = (mask_id >> 21);
        RXMnSIDL = ((mask_id >> 13) & SIDL_SID);
        RXMnSIDL |= ((mask_id >> 16) & SIDL_EID);
        RXMnEID8 = (mask_id >> 8);
        RXMnEID0 = mask_id;

        RXMnSIDL |= (1 << EXIDE);

        break;
    }

    default:
    {
        RXMnSIDH = (mask_id >> 3);
        RXMnSIDL = ((mask_id << 5) & SIDL_SID);
        RXMnEID8 = 0x00;
        RXMnEID0 = 0x00;

        break;
    }
    }

    MCP2515_SELECT();
    spi_write(address);
    spi_write(RXMnSIDH);
    spi_write(RXMnSIDL);
    spi_write(RXMnEID8);
    spi_write(RXMnEID0);
    MCP2515_UNSELECT();
}
