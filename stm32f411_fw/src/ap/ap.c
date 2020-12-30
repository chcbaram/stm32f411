/*
 * ap.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "ap.h"



void lcdMain(void);



void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);
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
    lcdMain();
  }
}

void lcdMain(void)
{
  static uint32_t pre_time;
  McpMode mode;
  McpBaud baud;
  int16_t x;
  int16_t y;

  if (lcdIsInit() != true)
  {
    return;
  }


  if (millis()-pre_time >= (1000/30) && lcdDrawAvailable() == true)
  {
    pre_time = millis();

    lcdClearBuffer(black);

    lcdPrintf(24,16*0, green, " [CAN 통신]");

    mode = mcp2515GetMode();
    baud = mcp2515GetBaud();

    x = 0;
    y = 16*1;
    switch(mode)
    {
      case MCP_MODE_NORMAL:
        lcdPrintf(x, y, white, "Mode : Normal");
        break;
      case MCP_MODE_SLEEP:
        lcdPrintf(x, y, white, "Mode : Sleep");
        break;
      case MCP_MODE_LOOPBACK:
        lcdPrintf(x, y, white, "Mode : Loopback");
        break;
      case MCP_MODE_LISTEN:
        lcdPrintf(x, y, white, "Mode : Listen");
        break;
      case MCP_MODE_CONFIG:
        lcdPrintf(x, y, white, "Mode : Config");
        break;
    }

    x = 0;
    y = 16*2;
    switch(baud)
    {
      case MCP_BAUD_125K:
        lcdPrintf(x, y, white, "Baud : 125Kbps");
        break;
      case MCP_BAUD_250K:
        lcdPrintf(x, y, white, "Baud : 250Kbps");
        break;
      case MCP_BAUD_500K:
        lcdPrintf(x, y, white, "Baud : 500Kbps");
        break;
      case MCP_BAUD_1000K:
        lcdPrintf(x, y, white, "Baud : 1Mbps");
        break;
    }


    lcdRequestDraw();
  }
}
