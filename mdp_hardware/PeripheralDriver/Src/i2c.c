/*
 * i2c.c
 *
 *  Created on: Aug 22, 2022
 *      Author: Zavier
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* variables ------------------------------------------------------------------*/
uint8_t i2cBuffer[20];
uint8_t ICMAddr = 0x68;

// Helper functions to transmit and receive data from ICM using I2C
// -------------------------------------------------------------------
void readByte(uint8_t addr, uint8_t *data) {
	i2cBuffer[0] = addr;
	HAL_I2C_Master_Transmit(&hi2c1, ICMAddr << 1, i2cBuffer, 1, 10);
	HAL_I2C_Master_Receive(&hi2c1, ICMAddr << 1, data, 2, 20);
}

void writeByte(uint8_t addr, uint8_t data) {
	i2cBuffer[0] = addr;
	i2cBuffer[1] = data;
	HAL_I2C_Master_Transmit(&hi2c1, ICMAddr << 1, i2cBuffer, 2, 20);
}
// -------------------------------------------------------------------
