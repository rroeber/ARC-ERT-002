#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "meter.h"
#include "battery.h"



void battery_voltage_init()
{
    adc_init();
    adc_gpio_init(VMEAS_PIN);
    adc_select_input(0);        // Select GPIO26 (also see VMEAS_PIN def in meter.h)

    gpio_init(VMEAS_EN_PIN);    // Initialize the enable pin as input so that the initial
                                // state is disabled
}

// Return the battery voltage as a float.  Through testing, determine if there is a significant
//   battery life extension using integer math to convert the ADC value to a battery voltage.
//   Speed is not very important in this application.
float get_battery_voltage()
{
    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    float battery_voltage = 0.0;
    uint16_t result = 0;   // Raw ADC value

    gpio_set_dir(VMEAS_EN_PIN, GPIO_OUT);
    gpio_put(VMEAS_EN_PIN, 1);   // Enable battery voltage to the ADC
    sleep_ms(10);

    // Take two readings
    result = adc_read();
    sleep_ms(10);
    result += adc_read();

    gpio_set_dir(VMEAS_EN_PIN, GPIO_IN);    // Disable battery voltage to ADC

    // Divide result by 2 to get the average of two readings
    result = result >> 1;

    battery_voltage = (result * 0.00125) + 0.080;
    //printf("Raw value: 0x%03x, voltage: %f V\n", result, battery_voltage);  // voltage divider conversion

    return battery_voltage;
}
