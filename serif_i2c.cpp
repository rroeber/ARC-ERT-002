/*
 * serif_i2c.c
 *
 *  Created on: Aug 10, 2020
 *      Author: r2
 */


/*  SDK Included Files */
#include <stdio.h>
#include "pico/stdlib.h"
#include "meter.h"       // For I2C port definition
#include "hardware/i2c.h"
#include "serif_i2c.h"

// Read data from an I2C slave
void serif_i2c_read_reg(uint8_t slave_address, uint8_t slave_reg, uint8_t *data, uint8_t length)
{
	uint8_t cmd = slave_reg;
	// First set the slave register pointer
	i2c_write_blocking(I2C_PORT, slave_address, &cmd, 1, true);

	// Read from the slave
	i2c_read_blocking (I2C_PORT, slave_address, data, length, false);
}


// Write data to an I2C slave
void serif_i2c_write_reg(uint8_t slave_address, uint8_t slave_reg, uint8_t *data, uint8_t length)
{
    // buf[0] is the register to write to
    // buf[1] and following are the value that will be written to the register

	uint8_t buf[16];
	uint8_t i;

	buf[0] = slave_reg;
	for (i=0; i<length; i++)
	{
		buf[i+1] = data[i];
	}

	i2c_write_blocking (I2C_PORT, slave_address, buf, length+1, false);   // Write data and release bus control
}