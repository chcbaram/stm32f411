/*
 * reset.c
 *
 *  Created on: 2020. 12. 9.
 *      Author: baram
 */


#include "reset.h"
#include "rtc.h"


#ifdef _USE_HW_RESET

static uint32_t reset_count = 0;



bool resetInit(void)
{
  bool ret = true;

#if 0
  // 만약 Reset 핀이 눌렸다면
  //
  if (RCC->CSR & (1<<26))
  {
    rtcBackupRegWrite(1, rtcBackupRegRead(1) + 1);
    delay(500);
    reset_count = rtcBackupRegRead(1);
  }

  rtcBackupRegWrite(1, 0);
#endif

  return ret;
}

uint32_t resetGetCount(void)
{
  return reset_count;
}


#endif
