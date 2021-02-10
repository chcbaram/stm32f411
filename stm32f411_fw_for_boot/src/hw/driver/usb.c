/*
 * usb.c
 *
 *  Created on: 2020. 12. 9.
 *      Author: baram
 */


#include "usb.h"
#include "cdc.h"

#ifdef _USE_HW_USB
#include "usbd_core.h"

#if HW_USE_CDC == 1
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#endif

#if HW_USE_MSC == 1
#include "usbd_msc.h"
#include "usbd_storage_if.h"
#endif


static bool is_init = false;
static UsbMode is_usb_mode = USB_NON_MODE;

USBD_HandleTypeDef hUsbDeviceFS;

extern USBD_DescriptorsTypeDef CDC_Desc;
extern USBD_DescriptorsTypeDef MSC_Desc;


bool usbInit(void)
{
  bool ret = true;



  return ret;
}

void usbDeInit(void)
{
  if (is_init == true)
  {
    USBD_DeInit(&hUsbDeviceFS);
  }
}

UsbMode usbGetMode(void)
{
  return is_usb_mode;
}

bool usbBegin(UsbMode usb_mode)
{
  bool ret = false;


#if HW_USE_CDC == 1

  if (usb_mode == USB_CDC_MODE)
  {
    if (USBD_Init(&hUsbDeviceFS, &CDC_Desc, DEVICE_FS) != USBD_OK)
    {
      return false;
    }
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK)
    {
      return false;
    }
    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK)
    {
      return false;
    }
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
    {
      return false;
    }

    cdcInit();

    is_usb_mode = USB_CDC_MODE;
    ret = true;
  }
#endif

#if HW_USE_MSC == 1

  if (usb_mode == USB_MSC_MODE)
  {
    if (USBD_Init(&hUsbDeviceFS, &MSC_Desc, DEVICE_FS) != USBD_OK)
    {
      return false;
    }
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_MSC) != USBD_OK)
    {
      return false;
    }
    if (USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS) != USBD_OK)
    {
      return false;
    }
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
    {
      return false;
    }

    is_usb_mode = USB_MSC_MODE;
    ret = true;
  }
#endif


  is_init = ret;

  return ret;
}


#endif
