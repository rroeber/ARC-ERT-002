#ifndef METER_H
#define METER_H


#define NUM_ENCODER_CHARS (uint8_t) 30     // TODO: Remove references to NUM_METER_PACKETS and use this


#define SERIAL_NUMBER_LENGTH 9          // Number of characters in the meter serial number
#define ODOMETER_LENGTH 6               // Number of meter odometer digits
#define TIMESTAMP_LENGTH 19             // timeStamp in ISO 8601 format yyyy-mm-ddThh:mm:ss

#define FIELD_SEPARATOR ','


#define I2C_PORT i2c0       // I2C port definition
#define I2C_SDA_PIN 12      // I2C SDA pin definition
#define I2C_SCL_PIN 13      // I2C SCL pin definition

#define RTC_INT_PIN 9       // RTC interrupt connected to GP9

#define VMEAS_PIN 26        // Battery voltage ADC measurement pin
#define VMEAS_EN_PIN 17     // Battery voltage measurement enable pin


// Use uart1 to receive encoder data.
#define UART_ID uart1
// #define UART_TX_PIN 4   // The UART xmit pin is not used with the encoder
#define UART_RX_PIN 5       // GP5 (pin 7) encoder data pin
#define ENCODER_CLK 7       // Use GP7 (pin 10) for the encoder clock pin


// SPI definitions.  See porting_functions.cpp
#define SPI_PORT       spi0
#define SPI_DATA_RATE  1000000
#define PIN_MISO 19
#define PIN_MOSI 16
#define PIN_SCK 18
#define RFM_CS 20
#define RFM_RESET 8
#define RFM_DIO0 11
#define RFM_DIO1 10

#endif  // METER_H