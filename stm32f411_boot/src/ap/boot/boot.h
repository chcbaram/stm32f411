/*
 * boot.h
 *
 *  Created on: 2021. 2. 4.
 *      Author: baram
 */

#ifndef SRC_AP_BOOT_BOOT_H_
#define SRC_AP_BOOT_BOOT_H_

#include "hw.h"




#define BOOT_ERR_WRONG_CMD      0x01
#define BOOT_ERR_LED            0x02
#define BOOT_ERR_FLASH_ERASE    0x03
#define BOOT_ERR_WRONG_RANGE    0x04
#define BOOT_ERR_FLASH_WRITE    0x05
#define BOOT_ERR_BUF_OVF        0x06
#define BOOT_ERR_INVALID_FW     0x07
#define BOOT_ERR_FW_CRC         0x08





void bootInit(void);
void bootProcessCmd(cmd_t *p_cmd);
bool bootVerifyFw(void);
bool bootVerifyCrc(void);
void bootJumpToFw(void);


#endif /* SRC_AP_BOOT_BOOT_H_ */
