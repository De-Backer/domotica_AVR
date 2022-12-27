
#include "../include/SPI.h"

void init_SPI()
{
    SPI_DDR |=
        (0x01 << SPI_CS) | (0x01 << SPI_SCK) | (0x01 << SPI_MOSI); /* output */
    SPI_DDR &= ~(0x01 << SPI_MISO);                                /* input */
    /* Enable SPI, Master, set clock rate fck/128 */
    //    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (1 << SPR1);
    /* Enable SPI, Master, set clock rate fck/16 */
    //	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
    /* Enable SPI, Master, set clock rate fck/4 */
    SPCR = (1 << SPE) | (1 << MSTR);
    /* Enable SPI, Master, set clock rate fck/2 */
    //    SPCR = (1 << SPE) | (1 << MSTR);
    //    SPSR |= (1 << SPI2X);
}

uint8_t spi_readwrite(uint8_t data)
{
    /* Start transmission */
    SPDR = data;
    /*
   * The following NOP introduces a small delay that can prevent the wait
   * loop form iterating when running at the maximum speed. This gives
   * about 10% more speed, even if it seems counter-intuitive. At lower
   * speeds it is unnoticed.
   */
  __asm__ __volatile__ ("nop");
    while (! (SPSR & (1 << SPIF)))
        ; /* Wait for transmission complete */
    return SPDR;
}

void spi_write(uint8_t data)
{
    /* Start transmission */
    SPDR = data;
    /*
   * The following NOP introduces a small delay that can prevent the wait
   * loop form iterating when running at the maximum speed. This gives
   * about 10% more speed, even if it seems counter-intuitive. At lower
   * speeds it is unnoticed.
   */
  __asm__ __volatile__ ("nop");
    while (! (SPSR & (1 << SPIF)))
        ; /* Wait for transmission complete */
}

uint8_t spi_read()
{
    /* Start transmission */
    SPDR = 0x00;
    /*
   * The following NOP introduces a small delay that can prevent the wait
   * loop form iterating when running at the maximum speed. This gives
   * about 10% more speed, even if it seems counter-intuitive. At lower
   * speeds it is unnoticed.
   */
  __asm__ __volatile__ ("nop");
    while (! (SPSR & (1 << SPIF)))
        ; /* Wait for transmission complete */
    return SPDR;
}
