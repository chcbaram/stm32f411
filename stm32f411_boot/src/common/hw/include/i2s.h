/*
 * i2s.h
 *
 *  Created on: 2021. 1. 9.
 *      Author: baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_I2S_H_
#define SRC_COMMON_HW_INCLUDE_I2S_H_

#include "hw_def.h"

#ifdef _USE_HW_I2S

#define I2S_MAX_CH       HW_I2S_MAX_CH

bool i2sInit(void);
bool i2sIsInit(void);
bool i2sStart(void);
bool i2sStop(void);

#endif

#endif /* SRC_COMMON_HW_INCLUDE_I2S_H_ */
