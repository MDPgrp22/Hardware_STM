/*
 * accel.c
 *
 *  Created on: Aug 24, 2022
 *      Author: Zavier
 */

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"
#include "main.h"
#include "i2c.h"

// Enable and configure ICM-20948 accelerometer
// -------------------------------------------------------------------
void accelInit(){
	writeByte(0x07, 0x38); // Disable accelerometer (all axes)
	osDelayUntil(10);

	writeByte(0x7F, 0x20); // Switch to USER BANK 2
	osDelayUntil(10);

	writeByte(0x14, 0x07); // Low pass filter: ACCEL_DLPFCFG = 0, ACCEL FS Select = 11, ACCEL_FCHOICE = 1 (Enable accel DLPF)
	osDelayUntil(10);

	//Set accel sample rate divider = 1125/1 + (ACCEL_SMPLRT_DIV[11:0])
	// Begin -------------------------------------------------------------------------------------
	writeByte(0x10, 0x00); // ACCEL_SMPLRT_DIV[11:8] -> Bit 3 to bit 0
	osDelayUntil(10);

	writeByte(0x11, 0x05); // ACCEL_SMPLRT_DIV[7:0]
	osDelayUntil(10);
	// End -------------------------------------------------------------------------------------

	writeByte(0x15, 0x03); // Set number of samples averaged in the accel decimator as 3: Average 32 samples
	osDelayUntil(10);

	writeByte(0x7F, 0x00); // Switch to USER BANK 0
	osDelayUntil(10);

	writeByte(0x07, 0x00); // Enable Gyroscope and Accelerometer
	osDelayUntil(10);
}
