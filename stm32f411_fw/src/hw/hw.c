/*
 * hw.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "hw.h"





void hwInit(void)
{
  bspInit();

  cliInit();
  ledInit();
  usbInit();
  uartInit();
  buttonInit();
  gpioInit();
  flashInit();
  spiInit();

  if (sdInit() == true)
  {
    fatfsInit();
  }

  if (buttonGetPressed(_DEF_BUTTON1) == true && sdIsDetected() == true)
  {
    usbBegin(USB_MSC_MODE);
  }
  else
  {
    usbBegin(USB_CDC_MODE);
  }

  lcdInit();
  mcp2515Init();
}
