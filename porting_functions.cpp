#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/structs/rosc.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "meter.h"

#include "porting_functions.h"

void pinMode(uint8_t pin, uint8_t dir)
{
    gpio_init(pin);   // Initialize for I/O, set to INPUT, clear any OUTPUT value
    if(dir == OUTPUT)
        gpio_set_dir(pin, GPIO_OUT);
    else
        gpio_set_dir(pin, GPIO_IN);  // TODO: Remove since it is rudundant
}

void pinPull(uint8_t pin, uint8_t dir)
{
    if(dir == PULL_UP)
        gpio_pull_up(pin);
    else if (dir == PULL_DOWN)
        gpio_pull_down(pin);
    else
        return;
}

void digitalWrite(uint8_t pin, uint8_t val)
{
    if(val == HIGH)
        gpio_put(pin, 1);
    else
        gpio_put(pin, 0);
}

uint8_t digitalRead(uint8_t pin)
{
    return(gpio_get(pin));
}

void delay(uint32_t mS)
{
    sleep_ms(mS);
}

uint32_t millis(void)
{
    return time_us_32() / 1000;  // Return lower 32 bits of time_us_64() times 1000
}


// Von Neumann extractor: From the input stream, his extractor took bits, two at a time (first and second, then third and fourth, and so on). If the two bits matched, no output was generated. If the bits differed, the value of the first bit was output. 
// https://forums.raspberrypi.com/viewtopic.php?t=302960

uint32_t seed_value()
{
    int k, random=0;
    int random_bit1, random_bit2;
    volatile uint32_t *rnd_reg=(uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
    
    for(k=0;k<32;k++){
        while(1){
            random_bit1=0x00000001 & (*rnd_reg);
            random_bit2=0x00000001 & (*rnd_reg);
            if(random_bit1 != random_bit2) break;
        }

	random = random << 1;
    random=random + random_bit1; 
    }

    return random;
}

/*
uint32_t seed_value(void)
{
    return time_us_32(); // Return lower 32 bits of time_us_64()
}
*/

uint32_t random(uint32_t seed)
{
    return rand();
}

uint32_t random_bounded(uint32_t low_val, uint32_t high_val)
{
    return rand() / (RAND_MAX / (high_val - low_val + 1) + low_val);
}

/************* SPI Functions ***********************/
void init_spi()
{
    // Set up the SPI parameters. Definitions are in meter.h
    // TODO: check that default is MSB First and SPI_MODE0
    spi_init(SPI_PORT, SPI_DATA_RATE);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);


    // Chip select is active-low -> initialize it to HIGH
    //   Note: CS pin is initialized in lorawan-arduino-rmf.cpp, lora_init()
    /*
    gpio_init(RFM_CS);
    gpio_put(RFM_CS, 1);
    gpio_set_dir(RFM, GPIO_OUT);
    */
}

// SPI_PORT is defined in meter.h
uint8_t SpiInOut(uint16_t outData)
{
    const uint8_t outDataB = (outData & 0xff);
    uint8_t inDataB = 0x00;

    spi_write_read_blocking(SPI_PORT, &outDataB, &inDataB, 1);

    return inDataB;
}

