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
#define BOOT_CMD_FLASH_ERASE            0x04
#define BOOT_CMD_FLASH_WRITE            0x05
#define BOOT_CMD_JUMP_TO_FW             0x08
#define BOOT_CMD_LED_CONTROL            0x10






extern firm_version_t boot_ver;

firm_version_t *p_boot_ver = &boot_ver;
firm_version_t *p_firm_ver = (firm_version_t *)(FLASH_ADDR_FW_VER);



static void bootCmdReadBootVersion(cmd_t *p_cmd);
static void bootCmdReadBootName(cmd_t *p_cmd);
static void bootCmdReadFirmVersion(cmd_t *p_cmd);
static void bootCmdReadFirmName(cmd_t *p_cmd);
static void bootCmdFlashErase(cmd_t *p_cmd);
static void bootCmdFlashWrite(cmd_t *p_cmd);
static void bootCmdJumpToFw(cmd_t *p_cmd);
static void bootCmdLedControl(cmd_t *p_cmd);


static bool bootIsFlashRange(uint32_t addr_begin, uint32_t length);




void bootInit(void)
{

}

bool bootVerifyFw(void)
{
  uint32_t *jump_addr = (uint32_t *)(FLASH_ADDR_FW + 4);


  if ((*jump_addr) >= FLASH_ADDR_START && (*jump_addr) <  FLASH_ADDR_END)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void bootJumpToFw(void)
{
  void (**jump_func)(void) = (void (**)(void))(FLASH_ADDR_FW + 4);

  bspDeInit();
  (*jump_func)();
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

    case BOOT_CMD_FLASH_ERASE:
      bootCmdFlashErase(p_cmd);
      break;

    case BOOT_CMD_FLASH_WRITE:
      bootCmdFlashWrite(p_cmd);
      break;

    case BOOT_CMD_JUMP_TO_FW:
      bootCmdJumpToFw(p_cmd);
      break;

    default:
      cmdSendResp(p_cmd, p_cmd->rx_packet.cmd, BOOT_ERR_WRONG_CMD, NULL, 0);
      break;
  }
}

void bootCmdReadBootVersion(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_BOOT_VERSION, CMD_OK, (uint8_t *)p_boot_ver->version, 32);
}

void bootCmdReadBootName(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_BOOT_NAME, CMD_OK, (uint8_t *)p_boot_ver->name, 32);
}

void bootCmdReadFirmVersion(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_FIRM_VERSION, CMD_OK, (uint8_t *)p_firm_ver->version, 32);
}

void bootCmdReadFirmName(cmd_t *p_cmd)
{
  cmdSendResp(p_cmd, BOOT_CMD_READ_FIRM_NAME, CMD_OK, (uint8_t *)p_firm_ver->name, 32);
}

void bootCmdFlashErase(cmd_t *p_cmd)
{
  uint8_t err_code = CMD_OK;
  uint32_t addr;
  uint32_t length;
  cmd_packet_t *p_packet;

  p_packet = &p_cmd->rx_packet;


  addr  = (uint32_t)(p_packet->data[0] <<  0);
  addr |= (uint32_t)(p_packet->data[1] <<  8);
  addr |= (uint32_t)(p_packet->data[2] << 16);
  addr |= (uint32_t)(p_packet->data[3] << 24);

  length  = (uint32_t)(p_packet->data[4] <<  0);
  length |= (uint32_t)(p_packet->data[5] <<  8);
  length |= (uint32_t)(p_packet->data[6] << 16);
  length |= (uint32_t)(p_packet->data[7] << 24);


  // 유효한 메모리 영역인지 확인.
  if (bootIsFlashRange(addr, length) == true)
  {
    // 메모리를 지움.
    if (flashErase(addr, length) != true)
    {
      err_code = BOOT_ERR_FLASH_ERASE;
    }
  }
  else
  {
    err_code = BOOT_ERR_WRONG_RANGE;
  }

  cmdSendResp(p_cmd, BOOT_CMD_FLASH_ERASE, err_code, NULL, 0);
}

void bootCmdFlashWrite(cmd_t *p_cmd)
{
  uint8_t err_code = CMD_OK;
  uint32_t addr;
  uint32_t length;
  cmd_packet_t *p_packet;

  p_packet = &p_cmd->rx_packet;


  addr  = (uint32_t)(p_packet->data[0] <<  0);
  addr |= (uint32_t)(p_packet->data[1] <<  8);
  addr |= (uint32_t)(p_packet->data[2] << 16);
  addr |= (uint32_t)(p_packet->data[3] << 24);

  length  = (uint32_t)(p_packet->data[4] <<  0);
  length |= (uint32_t)(p_packet->data[5] <<  8);
  length |= (uint32_t)(p_packet->data[6] << 16);
  length |= (uint32_t)(p_packet->data[7] << 24);


  // 유효한 메모리 영역인지 확인.
  if (bootIsFlashRange(addr, length) == true)
  {
    // 데이터를 Write.
    if (flashWrite(addr, &p_packet->data[8], length) != true)
    {
      err_code = BOOT_ERR_FLASH_WRITE;
    }
  }
  else
  {
    err_code = BOOT_ERR_WRONG_RANGE;
  }

  cmdSendResp(p_cmd, BOOT_CMD_FLASH_WRITE, err_code, NULL, 0);
}

void bootCmdJumpToFw(cmd_t *p_cmd)
{
  if (bootVerifyFw() == true)
  {
    cmdSendResp(p_cmd, BOOT_CMD_JUMP_TO_FW, CMD_OK, NULL, 0);
    delay(100);
    bootJumpToFw();
  }
  else
  {
    cmdSendResp(p_cmd, BOOT_CMD_JUMP_TO_FW, BOOT_ERR_INVALID_FW, NULL, 0);
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

bool bootIsFlashRange(uint32_t addr_begin, uint32_t length)
{
  bool ret = false;
  uint32_t addr_end;
  uint32_t flash_start;
  uint32_t flash_end;


  addr_end = addr_begin + length - 1;

  flash_start = FLASH_ADDR_START;
  flash_end   = FLASH_ADDR_END;

  if ((addr_begin >= flash_start) && (addr_begin < flash_end) &&
      (addr_end   >= flash_start) && (addr_end   < flash_end))
  {
    ret = true;
  }


  return ret;
}
