/*
 * boot.c
 *
 *  Created on: 2021. 2. 4.
 *      Author: baram
 */


#include "boot.h"


#define BOOT_CMD_LED_CONTROL          0x10




static void bootCmdLedControl(cmd_t *p_cmd);




void bootInit(void)
{

}

void bootProcessCmd(cmd_t *p_cmd)
{
  switch(p_cmd->rx_packet.cmd)
  {
    case BOOT_CMD_LED_CONTROL:
      bootCmdLedControl(p_cmd);
      break;

    default:
      cmdSendResp(p_cmd, p_cmd->rx_packet.cmd, BOOT_ERR_WRONG_CMD, NULL, 0);
      break;
  }
}




void bootCmdLedControl(cmd_t *p_cmd)
{
  uint8_t err_code = CMD_OK;
  cmd_packet_t *p_packet;


  p_packet = &p_cmd->rx_packet;

  if (p_packet->data[0] == 0)
  {
    ledOff(_DEF_LED1);
  }
  else if (p_packet->data[0] == 1)
  {
    ledOn(_DEF_LED1);
  }
  else if (p_packet->data[0] == 2)
  {
    ledToggle(_DEF_LED1);
  }
  else
  {
    err_code = BOOT_ERR_LED;
  }

  cmdSendResp(p_cmd, BOOT_CMD_LED_CONTROL, err_code, NULL, 0);
}
