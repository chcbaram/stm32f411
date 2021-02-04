/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"
#include "boot/boot.h"



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
      bootProcessCmd(&cmd);
    }
  }
}

