/*
 * usb.c
 *
 *  Created on: 2020. 12. 9.
 *      Author: baram
 */


#include "usb.h"


#ifdef _USE_HW_USB
#include "usb_device.h"


bool usbInit(void)
{
  bool ret = true;



  MX_USB_DEVICE_Init();

  return ret;
}


#endif
