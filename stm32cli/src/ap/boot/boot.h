/*
 * boot.h
 *
 *  Created on: 2021. 2. 6.
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




bool bootInit(uint8_t channel, char *port_name, uint32_t baud);
bool bootDeInit(uint8_t channel);


uint8_t bootCmdReadBootVersion(uint8_t *p_version);
uint8_t bootCmdReadBootName(uint8_t *p_str);
uint8_t bootCmdReadFirmVersion(uint8_t *p_version);
uint8_t bootCmdReadFirmName(uint8_t *p_str);
uint8_t bootCmdFlashErase(uint32_t addr, uint32_t length, uint32_t timeout);
uint8_t bootCmdFlashWrite(uint32_t addr, uint8_t *p_data, uint32_t length, uint32_t timeout);
uint8_t bootCmdJumpToFw(void);


#endif /* SRC_AP_BOOT_BOOT_H_ */
