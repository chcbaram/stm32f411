/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"



cmd_t cmd;


void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);

  cmdInit(&cmd);
  cmdOpen(&cmd, _DEF_UART1, 57600);
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
      //ledToggle(_DEF_LED1);
    }

    //cliMain();


    if (cmdReceivePacket(&cmd) == true)
    {
      uint8_t err_code = CMD_OK;

      //-- LED 제어 명령
      //
      if (cmd.rx_packet.cmd == 0x10)
      {
        if (cmd.rx_packet.data[0] == 0)
        {
          ledOff(_DEF_LED1);
        }
        else if (cmd.rx_packet.data[0] == 1)
        {
          ledOn(_DEF_LED1);
        }
        else if (cmd.rx_packet.data[0] == 2)
        {
          ledToggle(_DEF_LED1);
        }
        else
        {
          err_code = 0x01;
        }

        cmdSendResp(&cmd, 0x10, err_code, NULL, 0);
      }
    }
  }
}

