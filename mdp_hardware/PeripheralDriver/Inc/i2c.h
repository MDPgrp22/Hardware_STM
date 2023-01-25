/*
 * i2c.h
 *
 *  Created on: Aug 22, 2022
 *      Author: User
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Motor control functions ------------------------------------------------------------------*/
void readByte(uint8_t addr, uint8_t *data);
void writeByte(uint8_t addr, uint8_t data);

#endif /* INC_I2C_H_ */
