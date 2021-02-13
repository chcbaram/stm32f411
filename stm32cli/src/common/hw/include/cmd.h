/*
 * cmd.h
 *
 *  Created on: 2021. 1. 1.
 *      Author: baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_CMD_H_
#define SRC_COMMON_HW_INCLUDE_CMD_H_

#include "hw_def.h"

#ifdef _USE_HW_CMD



#define CMD_MAX_DATA_LENGTH       HW_CMD_MAX_DATA_LENGTH


#define CMD_OK                0
#define CMD_ERR_DIR           254
#define CMD_TIMEOUT           255


#define CMD_DIR_M_TO_S        0
#define CMD_DIR_S_TO_M        1



typedef struct
{
  uint8_t   cmd;
  uint8_t   dir;
  uint8_t   error;
  uint16_t  length;
  uint8_t   check_sum;
  uint8_t   check_sum_recv;
  uint8_t   buffer[CMD_MAX_DATA_LENGTH+8];
  uint8_t   *data;
} cmd_packet_t;


typedef struct
{
  uint8_t   ch;
  bool      is_init;
  uint32_t  baud;
  uint8_t   state;
  uint32_t  pre_time;
  uint32_t  index;
  uint8_t   error;

  cmd_packet_t  rx_packet;
  cmd_packet_t  tx_packet;
} cmd_t;


void cmdInit(cmd_t *p_cmd);
bool cmdOpen(cmd_t *p_cmd, uint8_t ch, uint32_t baud);
bool cmdClose(cmd_t *p_cmd);
bool cmdReceivePacket(cmd_t *p_cmd);
void cmdSendCmd(cmd_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length);
void cmdSendResp(cmd_t *p_cmd, uint8_t cmd, uint8_t err_code, uint8_t *p_data, uint32_t length);
bool cmdSendCmdRxResp(cmd_t *p_cmd, uint8_t cmd, uint8_t *p_data, uint32_t length, uint32_t timeout);

#endif

#endif /* SRC_COMMON_HW_INCLUDE_CMD_H_ */
