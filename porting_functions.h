#ifndef HELPERS_H
#define HELPERS_H


#include <stdint.h>
#include <stdbool.h>

#define HIGH (uint8_t) 1
#define LOW (uint8_t) 0

#define PULL_UP (uint8_t) 1
#define PULL_DOWN (uint8_t) 0

#define OUTPUT 0
#define INPUT 1

typedef uint8_t byte;

void digitalWrite(uint8_t pin, uint8_t val);
uint8_t digitalRead(uint8_t pin);
void delay(uint32_t mS);
void analogWrite(uint8_t pin, uint8_t val);
void pinMode(uint8_t pin, uint8_t dir);
void pinPull(uint8_t pin, uint8_t dir);
uint32_t millis(void);
uint32_t random(uint32_t seed);
uint32_t random_bounded(uint32_t low_val, uint32_t high_val);
uint32_t seed_value(void);

void init_spi();
uint8_t SpiInOut(uint16_t outData);

#endif   // #ifndef HELPERS_H
