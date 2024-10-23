#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "meter.h"
#include "encoder.h"


#include <stdio.h>


#define CLOCK_RATE 1024     // Data communications clock speed.
                            //   The Riva module uses 1024 Hz
//#define DATA_BITS 7
//#define STOP_BITS 1
//#define PARITY    UART_PARITY_EVEN

#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE



void encoder_uart_init()
{
    // Initialize the UART
    uart_init(UART_ID, CLOCK_RATE);

    // Set the TX and RX pins by using the GPIO function select
    //gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);  // Not needed
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    gpio_pull_up(UART_RX_PIN);  // The encoder has open drain output

    // Turn off UART hardware flow control
    //uart_set_hw_flow(UART_ID, false, false);

    // Set the data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's
    uart_set_fifo_enabled(UART_ID, false);
}

// https://www.i-programmer.info/programming/hardware/14849-the-pico-in-c-basic-pwm.html?start=1
/*
As already stated, the wrap value determines the frequency. It doesn’t take too much algebra to
work out that in non-phase-correct mode:

f = fc /(wrap+1)

where fc is the frequency of the clock after any division has been taken into account. More
usefully you can work out the wrap needed to give any PWM frequency:

wrap = fc/f - 1

Similarly, the level sets the duty cycle:

level = wrap * duty

where duty is the duty cycle as a fraction.

For example, with a clock frequency of 125MHz in non-phase-correct mode you can generate a
PWM signal of 10kHz using a wrap of 12,500 and a 25% duty cycle implies a level of 3,125.
*/
// https://www.iopress.info/index.php/books/programming-the-raspberry-pi-pico-in-c/9-programs/52-picocprograms

uint32_t pwm_set_freq_duty(uint slice_num, uint chan,
                           uint32_t f, int d)
{
    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + (clock % (f * 4096) != 0);
    if (divider16 / 16 == 0)
        divider16 = 16;
    uint32_t wrap = clock * 16 / divider16 / f - 1;
    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap * d / 100);
    return wrap;
}


uint encoder_pwm_init()
{
    gpio_set_function(ENCODER_CLK, GPIO_FUNC_PWM);
 
    uint slice_num = pwm_gpio_to_slice_num(ENCODER_CLK);
    uint chan_encoder_clk = pwm_gpio_to_channel(ENCODER_CLK);

    uint wrap = pwm_set_freq_duty(slice_num, chan_encoder_clk, 1024, 50);

    pwm_set_enabled(slice_num, true);

    return slice_num;
}


// Turn off, shut down and/or disable the interfaces to the encoder
void encoder_shutdown(uint pwm_slice_num)
{
    pwm_set_enabled(pwm_slice_num, false);  // Disable the pwm
    gpio_init(ENCODER_CLK);                 // Initialize the ENCODER_CLK pin (again) so that is 
                                            //    configured as a GPIO, input.
    gpio_pull_down(ENCODER_CLK);            // Ensure the pin is LOW to disable clk and power to the Encoder

    gpio_pull_down(UART_RX_PIN);            // Pull the receive data pin LOW
    uart_deinit(UART_ID);                   // Deinit the UART port
}


// Receives a character from the encoder
//    The character is 10 bits (start bit, 7E1) and the clock is 1024 bps
//    Receiving the character should take 9.8mS.  Whenever power/clk is first
//    applied to the encoder there is a startup delay.  Allow 1.5 seconds for
//    an error condition.
//    If a character is not received the
//    encoder is probably missing or broken.
char encoder_getchar()
{
    char recv_val = 0;     // Initialize return value to 0 indicating no received character
    uint32_t timeout_cnt = 0;

    while(recv_val == 0)
    {
        if (uart_is_readable(UART_ID))
        {
            recv_val = uart_getc(UART_ID) & 0x7f;
            break;
        }
        else
        {
            sleep_ms(1);
            timeout_cnt++;
            if(timeout_cnt > 1500)
            {
                printf("     Error! encoder_getchar() did not receive encoder character\n");
                break;
            }
        }
    }

    return recv_val;
}


// Reads NUM_METER_PACKETS from the encoder and
//   stores the data in the array that is passed to the function.
void encoder_read(uint8_t *encoder_data)
{
    uint8_t i = 0;              // loop counter
    uint8_t j = 0;              // loop counter
    uint8_t timeout = 0;        // Timeout for detecting encoder
    char encoder_char = 0;    // Character received from the encoder
    uint pwm_slice_num = 0;

    encoder_uart_init();

    // Ensure the receive buffer is clear before reading the meter data
    if (uart_is_readable(UART_ID))
    {
        encoder_char = encoder_getchar();
    }

    pwm_slice_num = encoder_pwm_init();
 
    // Initialize the return data array to ASCII 0 (0x30)
    for(i=0; i<NUM_ENCODER_CHARS; i++)
    {
        encoder_data[i] = 0x30;
    }

    // Read the encoder data.  The encoder data packet is 31 characters.  It begins with 'V'
    // and ends with 0x0d (carriage return).  The data packet is sent 4 times for a 
    // total of 124 characters.  

    // First, locate the initial character - 'V'.  The 'V' occurs at character 0, 32, 64 and 96.
    // If the 'V' is not identified, return an error.  The encoder is probably present since
    // characters are received but is probably broken since the 'V' is not identified.


    for(i=0; i<97; i++)
    {
        encoder_char = encoder_getchar();
        if(encoder_char == 'V')    // Found the start-of-packet character
        {
            encoder_data[0] = encoder_char;
            break;
        }
        else if(encoder_char == 0)  // Error - no character received from the encoder
        {
            break;
        }
        sleep_ms(1);
    }

    if(encoder_data[0] != 'V')
    {
        encoder_shutdown(pwm_slice_num);
        return;     // TODO: Restructure the function to return a uint8_t error code.  For now, the return array
                    //  is all ASCII 0 which can be used as an error condition.
    }

    // Now read the remaining characters (characters 1 through NUM_ENCODER_CHARS)
    for(i=1; i<NUM_ENCODER_CHARS; i++)
    {
        encoder_char = encoder_getchar();
        if(encoder_char == 0)
        {
            for(j=1; j<NUM_ENCODER_CHARS; j++)
            {
                encoder_data[j] = 0x30;
            }
            printf("Error reading after the V\n");
            break;     // If the encoder_char is 0, the encoder stopped working.  Very unlikely, but return with
                        // the 'V' in encoder_data[0] and all other values ASCII 0, 0x30.
        }
        else
        {
            encoder_data[i] = encoder_char;
        }
    }

    encoder_shutdown(pwm_slice_num);  // Disable the encoder interface and return
 }


