/*
 * hw.c
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */


#include "hw.h"



__attribute__((section(".version"))) firm_version_t firm_ver =
    {
        "V210211R2",
        "Firmware"
    };


void hwInit(void)
{
  bspInit();

  rtcInit();
  resetInit();
  cliInit();
  ledInit();
  usbInit();
  uartInit();
  buttonInit();
  gpioInit();
  flashInit();

  usbBegin(USB_CDC_MODE);
}
