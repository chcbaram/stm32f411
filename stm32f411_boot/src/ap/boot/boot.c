/*
 * boot.c
 *
 *  Created on: 2021. 2. 4.
 *      Author: baram
 */


#include "boot.h"


#define BOOT_CMD_READ_BOOT_VERSION      0x00
#define BOOT_CMD_READ_BOOT_NAME         0x01
#define BOOT_CMD_READ_FIRM_VERSION      0x02
#define BOOT_CMD_READ_FIRM_NAME         0x03
#define BOOT_CMD_LED_CONTROL            0x10





const uint8_t boot_ver[32]  = "B210206R1";
const uint8_t boot_name[32] = "STM32F411";
const uint8_t firm_ver[32]  = "V210206R1";
const uint8_t firm_name[32] = "STM32F411";


static void bootCmdReadBootVersion(cmd_t *p_cmd);
static void bootCmdReadBootName(cmd_t *p_cmd);
static void bootCmdReadFirmVersion(cmd_t *p_cmd);
static void bootCmdReadFirmName(cmd_t *p_cmd);
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

    case BOOT_CMD_READ_BOOT_VERSION:
      bootCmdReadBootVersion(p_cmd);
      break;

    case BOOT_CMD_READ_BOOT_NAME:
      bootCmdReadBootName(p_cmd);
      break;

    case BOOT_CMD_READ_FIRM_VERSION:
      bootCmdReadFirmVersion(p_cmd);
      break;

    case BOOT_CMD_READ_FIRM_NAME:
      bootCmdReadFirmName(p_cmd);
      break;

    default:
      cmdSendResp(p_cmd, p_cmd->rx_packet.cmd, BOOT_ERR_WRONG_CMD, NULL, 0);
      break;
  }
}

void bootCmdReadBootVersion(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_BOOT_VERSION, CMD_OK, (uint8_t *)boot_ver, 32);
}

void bootCmdReadBootName(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_BOOT_NAME, CMD_OK, (uint8_t *)boot_name, 32);
}

void bootCmdReadFirmVersion(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_FIRM_VERSION, CMD_OK, (uint8_t *)firm_ver, 32);
}

void bootCmdReadFirmName(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_FIRM_NAME, CMD_OK, (uint8_t *)firm_name, 32);
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
