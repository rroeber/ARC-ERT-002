/*
 * serif_i2c.h
 *
 *  Created on: Aug 10, 2020
 *      Author: r2
 */

#ifndef SERIF_I2C_H_
#define SERIF_I2C_H_

#include <stdint.h>  // Needed for uint8_t definition

#if defined(__cplusplus)
extern "C" {
#endif

void serif_i2c_read_reg(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len);
void serif_i2c_write_reg(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len);

#if defined(__cplusplus)
}
#endif

#endif /* SERIF_I2C_H_ */
