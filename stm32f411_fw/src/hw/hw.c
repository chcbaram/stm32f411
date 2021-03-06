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
  i2cInit();
  i2sInit();

  if (sdInit() == true)
  {
    fatfsInit();
  }

  lcdInit();
  mcp2515Init();


  if (buttonGetPressed(_DEF_BUTTON1) == true && sdIsDetected() == true)
  {
    usbBegin(USB_MSC_MODE);
  }
  else
  {
    usbBegin(USB_CDC_MODE);
  }
}
