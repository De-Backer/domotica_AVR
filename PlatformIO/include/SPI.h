#ifndef SPI_H
#define SPI_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <avr/io.h>

/* default */
#ifndef SPI_PORT
#define SPI_PORT PORTB  /**< voor de MOSI & SCK */
#endif

#ifndef SPI_PIN
#define SPI_PIN  PINB   /**< voor de MISO       */
#endif

#ifndef SPI_DDR
#define SPI_DDR  DDRB   /**< voor sw_SPI_setup  */
#endif

#ifndef SPI_CS
#define SPI_CS   3      /**< voor sw_SPI_setup  */
#endif

#ifndef SPI_SCK
#define SPI_SCK  0      /**< SCK  <> SCK µc     */
#endif

#ifndef SPI_MOSI
#define SPI_MOSI 1      /**< MOSI <> MOSI µc    */
#endif

#ifndef SPI_MISO
#define SPI_MISO 6      /**< MISO <> MISO µc    */
#endif

    void    init_SPI();
    uint8_t spi_readwrite(uint8_t data);
    void    spi_write(uint8_t data);
    uint8_t spi_read();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SPI_H
