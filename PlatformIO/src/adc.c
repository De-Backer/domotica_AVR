#include "../include/adc.h"

//#include <util/delay.h>

void init_ADC()
{
    /* DIDR0 – Digital Input Disable Register 0 */
    DIDR0 = 0xff;

    /* ADC Multiplexer Selection Register */
        /* AREF, Internal Vref turned 2.56V */
        //ADMUX = (0x01 << REFS1) | (0x01 << REFS0);

        /* AVCC with external capacitor at AREF pin */
        //ADMUX=(0x01 << REFS0);

        /* MUX4:0: Analog Channel and Gain Selection Bits => ADC0 */
        /* ADLAR ADC Left Adjust Result */
        //ADMUX |= (0x01 << ADLAR);

        ADMUX = (0x01 << ADLAR);

    /* ADC Control and Status Register A */
        /* ADPS2:0: ADC Prescaler Select Bits 156.25KHz sample rate @ 20MHz */
        ADCSRA = (0x01 << ADPS2) | (0x01 << ADPS1) | (0x01 << ADPS0);
        /* ADEN: ADC Enable */
        ADCSRA |= (0x01 << ADEN);
        /* ADIE: Interrupt Enable */
        //ADCSRA |= (0x01 << ADIE);
}
uint8_t ADC_value_8bit(uint8_t source)
{
    source &= 0b00000111;
    source |= (ADMUX & 0b11100000);
    ADMUX = source;
    uint8_t data = 0;

    //    _delay_ms(2);
    /* Sleep Enable */
    SMCR |= (0x01 << SE);
    __asm__("SLEEP");

    data = ADCL; /* ADCL must be read first, then ADCH ref 23.9.3 */
    return ADCH;
}
uint16_t ADC_value(uint8_t source)
{
    source &= 0b00000111;
    ADMUX &= 0b11100000; // 0
    ADMUX |= source;
    uint16_t data = 0;

    //    _delay_ms(2);
    /* Sleep Enable */
    SMCR |= (0x01 << SE);
    __asm__("SLEEP");

    data = ADCL; /* ADCL must be read first, then ADCH ref 23.9.3 */
    data |= (ADCH << 2);
    return data;

    //    uint16_t data  = 0;
    //    uint16_t data2 = 0;
    //    uint16_t data3 = 0;
    //    _delay_ms(2);
    //    /* Sleep Enable */
    //    SMCR |= (0x01 << SE);
    //    __asm__("SLEEP");

    //    data = ADCL; /* ADCL must be read first, then ADCH ref 23.9.3 */
    //    data |= (ADCH << 2);

    //    uint8_t var = 0;
    //    for (; var < 2; ++var)
    //    {
    //        uint8_t var1 = 0;
    //        for (; var1 < 10; ++var1)
    //        {
    //            /* Sleep Enable */
    //            SMCR |= (0x01 << SE);
    //            __asm__("SLEEP");

    //            data = ADCL; /* ADCL must be read first, then ADCH ref 23.9.3
    //            */ data |= (ADCH << 2); data2 += data;
    //        }

    //        data3 += (data2 >> 0);
    //    }
    //    return data3;
}
/* interrupt */
ISR(ADC_vect)
{
    /* Sleep clear */
    SMCR &= ~(0x01 << SE);
}
