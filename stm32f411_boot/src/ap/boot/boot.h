/*
 * boot.h
 *
 *  Created on: 2021. 2. 4.
 *      Author: baram
 */

#ifndef SRC_AP_BOOT_BOOT_H_
#define SRC_AP_BOOT_BOOT_H_

#include "hw.h"




#define BOOT_ERR_WRONG_CMD    0x01
#define BOOT_ERR_LED          0x02





void bootInit(void);
void bootProcessCmd(cmd_t *p_cmd);


#endif /* SRC_AP_BOOT_BOOT_H_ */
