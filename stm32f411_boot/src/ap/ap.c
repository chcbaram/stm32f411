/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"



void cliBoot(cli_args_t *args);


void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);

  cliAdd("boot", cliBoot);
}

void apMain(void)
{
  uint32_t pre_time;


  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    cliMain();
  }
}

bool bootDownload(void)
{
  bool ret = false;
  bool keep_loop = true;
  uint8_t err_code = 0;
  uint32_t pre_time;
  ymodem_t ymodem;


  ymodemOpen(&ymodem, _DEF_UART1);


  cliPrintf("download ...");
  pre_time = millis();
  while(keep_loop)
  {
    if (ymodemReceive(&ymodem) == true)
    {
      pre_time = millis();

      switch(ymodem.type)
      {
        case YMODEM_TYPE_START:
          if (flashErase(FLASH_ADDR_FW, ymodem.file_length) != true)
          {
            keep_loop = false;
            err_code = 1;
          }
          break;

        case YMODEM_TYPE_DATA:
          if (flashWrite(FLASH_ADDR_FW + ymodem.file_addr, ymodem.file_buf, ymodem.file_buf_length) != true)
          {
            keep_loop = false;
            err_code = 2;
          }
          break;

        case YMODEM_TYPE_END:
          ret = true;
          keep_loop = false;
          break;

        case YMODEM_TYPE_CANCEL:
          keep_loop = false;
          err_code = 3;
          break;

        case YMODEM_TYPE_ERROR:
          keep_loop = false;
          err_code = 4;
          break;
      }

      ymodemAck(&ymodem);
    }
    if (millis()-pre_time >= 15*1000)
    {
      keep_loop = false;
      err_code = 5;
    }
  }


  if (ret == true)
  {
    cliPrintf("OK\n");
  }
  else
  {
    cliPrintf("Fail %d\n", err_code);
  }

  return ret;
}

void cliBoot(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("Flash Start : 0x%X\n", FLASH_ADDR_FW);
    cliPrintf("Flash End   : 0x%X\n", FLASH_ADDR_END);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "down_fw"))
  {
    bootDownload();
    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("boot info\n");
    cliPrintf("boot down_fw\n");
  }
}
